#include "MapStyle_P.h"
#include "MapStyle.h"

#include <cassert>
#include <iostream>

#include <OsmAndCore/QtExtensions.h>
#include <QStringList>
#include <QByteArray>
#include <QBuffer>
#include <QFileInfo>
#include <QStack>

#include "MapStyles.h"
#include "MapStyleRule.h"
#include "MapStyleRule_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "MapStyleConfigurableInputValue.h"
#include "EmbeddedResources.h"
#include "Logging.h"
#include "QKeyValueIterator.h"

OsmAnd::MapStyle_P::MapStyle_P( MapStyle* owner_ )
    : owner(owner_)
    , _builtinValueDefs(MapStyle::getBuiltinValueDefinitions())
    , _firstNonBuiltinValueDefinitionIndex(0)
    , _stringsIdBase(0)
{
    registerBuiltinValueDefinitions();
}

OsmAnd::MapStyle_P::~MapStyle_P()
{
}

bool OsmAnd::MapStyle_P::parseMetadata()
{
    if(owner->isEmbedded)
    {
        QXmlStreamReader data(EmbeddedResources::decompressResource(owner->resourcePath));
        return parseMetadata(data);
    }
    else
    {
        QFile styleFile(owner->resourcePath);
        if(!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QXmlStreamReader data(&styleFile);
        bool ok = parseMetadata(data);
        styleFile.close();
        return ok;
    }

    return false;
}

bool OsmAnd::MapStyle_P::parseMetadata( QXmlStreamReader& xmlReader )
{
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == QLatin1String("renderingStyle"))
            {
                _title = xmlReader.attributes().value(QLatin1String("name")).toString();
                auto attrDepends = xmlReader.attributes().value(QLatin1String("depends"));
                if(!attrDepends.isNull())
                    _parentName = attrDepends.toString();
            }
        }
        else if (xmlReader.isEndElement())
        {
        }
    }
    if(xmlReader.hasError())
    {
        std::cerr << qPrintable(xmlReader.errorString()) << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << ")" << std::endl;
        return false;
    }

    return true;
}

bool OsmAnd::MapStyle_P::resolveDependencies()
{
    if(_parentName.isEmpty())
        return true;

    // Make sure parent is resolved before this style (if present)
    if(!_parentName.isEmpty() && !_parent)
    {
        if(!owner->styles->obtainStyle(_parentName, _parent))
            return false;
    }

    // Obtain string ID base
    _stringsIdBase = _parent->_d->_stringsIdBase + _parent->_d->_stringsLUT.size();

    return true;
}

bool OsmAnd::MapStyle_P::parse()
{
    if(owner->isEmbedded)
    {
        QXmlStreamReader data(EmbeddedResources::decompressResource(owner->resourcePath));
        return parse(data);
    }
    else
    {
        QFile styleFile(owner->resourcePath);
        if(!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QXmlStreamReader data(&styleFile);
        bool ok = parse(data);
        styleFile.close();
        return ok;
    }

    return false;
}

bool OsmAnd::MapStyle_P::parse( QXmlStreamReader& xmlReader )
{
    auto rulesetType = MapStyleRulesetType::Invalid;

    struct Lexeme
    {
        enum Type
        {
            Rule,
            Group,
        };

        Lexeme(Type type_, MapStyle_P* style_)
            : type(type_)
            , style(style_)
        {}

        const Type type;
        MapStyle_P* const style;
    };

    struct Rule : public Lexeme
    {
        Rule(std::shared_ptr<MapStyleRule> rule_, MapStyle_P* style_)
            : Lexeme(Lexeme::Rule, style_)
            , rule(rule_)
        {}

        const std::shared_ptr<MapStyleRule> rule;
    };

    struct Group : public Lexeme
    {
        Group(MapStyle_P* const style_)
            : Lexeme(Lexeme::Group, style_)
        {}

        QHash< QString, QString > attributes;
        QList< std::shared_ptr<MapStyleRule> > children;
        QList< std::shared_ptr<Lexeme> > subgroups;

        void addGroupFilter(const std::shared_ptr<MapStyleRule>& rule)
        {
            for(const auto& child : constOf(children))
                child->_d->_ifChildren.push_back(rule);

            for(const auto& subgroup : constOf(subgroups))
            {
                assert(subgroup->type == Lexeme::Group);
                std::static_pointer_cast<Group>(subgroup)->addGroupFilter(rule);
            }
        }

        bool registerGlobalRules(const MapStyleRulesetType type)
        {
            for(const auto& child : constOf(children))
            {
                if(!style->registerRule(type, child))
                    return false;
            }

            for(const auto& subgroup : constOf(subgroups))
            {
                assert(subgroup->type == Lexeme::Group);
                if(!std::static_pointer_cast<Group>(subgroup)->registerGlobalRules(type))
                    return false;
            }

            return true;
        }
    };

    QStack< std::shared_ptr<Lexeme> > stack;

    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if(tagName == QLatin1String("renderingStyle"))
            {
                // We have already parsed metadata and resolved dependencies, so here we don't need to do anything
            }
            else if(tagName == QLatin1String("renderingConstant"))
            {
                const auto& attribs = xmlReader.attributes();

                auto name = attribs.value(QLatin1String("name")).toString();
                auto value = attribs.value(QLatin1String("value")).toString();
                _parsetimeConstants.insert(name, value);
            }
            else if(tagName == QLatin1String("renderingProperty"))
            {
                MapStyleConfigurableInputValue* inputValue = nullptr;

                const auto& attribs = xmlReader.attributes();
                auto name = obtainValue(attribs.value(QLatin1String("attr")).toString());
                auto type = obtainValue(attribs.value(QLatin1String("type")).toString());
                auto title = obtainValue(attribs.value(QLatin1String("name")).toString());
                auto description = attribs.value(QLatin1String("description")).toString();
                auto encodedPossibleValues = attribs.value(QLatin1String("possibleValues"));
                QStringList possibleValues;
                if(!encodedPossibleValues.isEmpty())
                    possibleValues = encodedPossibleValues.toString().split(',', QString::SkipEmptyParts);
                for(auto& possibleValue : possibleValues)
                    possibleValue = obtainValue(possibleValue);
                if(type == QLatin1String("string"))
                {
                    inputValue = new MapStyleConfigurableInputValue(
                        MapStyleValueDataType::String,
                        name,
                        title,
                        description,
                        possibleValues);
                }
                else if(type == QLatin1String("boolean"))
                {
                    inputValue = new MapStyleConfigurableInputValue(
                        MapStyleValueDataType::Boolean,
                        name,
                        title,
                        description,
                        possibleValues);
                }
                else // treat as integer
                {
                    inputValue = new MapStyleConfigurableInputValue(
                        MapStyleValueDataType::Integer,
                        name,
                        title,
                        description,
                        possibleValues);
                }

                registerValue(inputValue);
            }
            else if(tagName == QLatin1String("renderingAttribute"))
            {
                const auto& attribs = xmlReader.attributes();

                auto name = obtainValue(attribs.value(QLatin1String("name")).toString());

                std::shared_ptr<MapStyleRule> attribute(new MapStyleRule(owner, QHash< QString, QString >()));
                _attributes.insert(name, attribute);

                std::shared_ptr<Lexeme> selector(new Rule(attribute, this));
                stack.push(qMove(selector));
            }
            else if(tagName == QLatin1String("filter"))
            {
                QHash< QString, QString > ruleAttributes;

                if(!stack.isEmpty())
                {
                    auto lexeme = stack.top();

                    if(lexeme->type == Lexeme::Group)
                        ruleAttributes.unite(std::static_pointer_cast<Group>(lexeme)->attributes);
                }

                const auto& attribs = xmlReader.attributes();
                for(const auto& xmlAttrib : constOf(attribs))
                {
                    const auto tag = xmlAttrib.name().toString();
                    const auto value = obtainValue(xmlAttrib.value().toString());
                    ruleAttributes.insert(qMove(tag), qMove(value));
                }

                const std::shared_ptr<MapStyleRule> rule(new MapStyleRule(owner, ruleAttributes));

                if(!stack.isEmpty())
                {
                    auto lexeme = stack.top();

                    if(lexeme->type == Lexeme::Group)
                    {
                        std::static_pointer_cast<Group>(lexeme)->children.push_back(rule);
                    }
                    else if(lexeme->type == Lexeme::Rule)
                    {
                        std::static_pointer_cast<Rule>(lexeme)->rule->_d->_ifElseChildren.push_back(rule);
                    }
                }
                else
                {
                    if(!registerRule(rulesetType, rule))
                        return false;
                }

                stack.push(qMove(std::shared_ptr<Lexeme>(new Rule(rule, this))));
            }
            else if(tagName == QLatin1String("groupFilter"))
            {
                QHash< QString, QString > attributes;

                const auto& attribs = xmlReader.attributes();
                for(const auto& xmlAttrib : constOf(attribs))
                {
                    const auto tag = xmlAttrib.name().toString();
                    const auto value = obtainValue(xmlAttrib.value().toString());
                    attributes.insert(qMove(tag), qMove(value));
                }

                std::shared_ptr<MapStyleRule> rule(new MapStyleRule(owner, attributes));

                if(stack.isEmpty())
                {
                    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Group filter without parent");
                    return false;
                }

                auto lexeme = stack.top();
                if(lexeme->type == Lexeme::Group)
                {
                    std::static_pointer_cast<Group>(lexeme)->addGroupFilter(rule);
                }
                else if(lexeme->type == Lexeme::Rule)
                {
                    std::static_pointer_cast<Rule>(lexeme)->rule->_d->_ifChildren.push_back(rule);
                }

                stack.push(qMove(std::shared_ptr<Lexeme>(new Rule(rule, this))));
            }
            else if(tagName == QLatin1String("group"))
            {
                const auto group = new Group(this);
                if(!stack.isEmpty())
                {
                    auto lexeme = stack.top();
                    if(lexeme->type == Lexeme::Group)
                        group->attributes.unite(std::static_pointer_cast<Group>(lexeme)->attributes);
                }

                const auto& attribs = xmlReader.attributes();
                for(const auto& xmlAttrib : constOf(attribs))
                {
                    const auto tag = xmlAttrib.name().toString();
                    const auto value = obtainValue(xmlAttrib.value().toString());
                    group->attributes.insert(qMove(tag), qMove(value));
                }

                stack.push(qMove(std::shared_ptr<Lexeme>(group)));
            }
            else if(tagName == QLatin1String("order"))
            {
                rulesetType = MapStyleRulesetType::Order;
            }
            else if(tagName == QLatin1String("text"))
            {
                rulesetType = MapStyleRulesetType::Text;
            }
            else if(tagName == QLatin1String("point"))
            {
                rulesetType = MapStyleRulesetType::Point;
            }
            else if(tagName == QLatin1String("line"))
            {
                rulesetType = MapStyleRulesetType::Polyline;
            }
            else if(tagName == QLatin1String("polygon"))
            {
                rulesetType = MapStyleRulesetType::Polygon;
            } 
        }
        else if(xmlReader.isEndElement())
        {
            if(tagName == QLatin1String("filter"))
            {
                stack.pop();
            }
            else if(tagName == QLatin1String("group"))
            {
                const auto lexeme = stack.pop();

                if (stack.isEmpty())
                {
                    assert(lexeme->type == Lexeme::Group);
                    if(!std::static_pointer_cast<Group>(lexeme)->registerGlobalRules(rulesetType))
                        return false;
                }
                else
                {
                    const auto group = stack.top();
                    if(group->type == Lexeme::Group)
                        std::static_pointer_cast<Group>(group)->subgroups.push_back(qMove(lexeme));
                }
            }
            else if(tagName == QLatin1String("groupFilter"))
            {
                stack.pop();
            }
            else if(tagName == QLatin1String("renderingAttribute"))
            {
                stack.pop();
            }
        }

    }
    if(xmlReader.hasError())
    {
        std::cerr << qPrintable(xmlReader.errorString()) << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << ")" << std::endl;
        return false;
    }

    return true;
}

void OsmAnd::MapStyle_P::registerBuiltinValueDefinition( const std::shared_ptr<const MapStyleValueDefinition>& valueDefinition )
{
    assert(_valuesDefinitions.size() == 0 || _valuesDefinitions.size() <= _firstNonBuiltinValueDefinitionIndex);

    _valuesDefinitions.insert(valueDefinition->name, valueDefinition);
    _firstNonBuiltinValueDefinitionIndex = _valuesDefinitions.size();
}

std::shared_ptr<const OsmAnd::MapStyleValueDefinition> OsmAnd::MapStyle_P::registerValue( const MapStyleValueDefinition* pValueDefinition )
{
    std::shared_ptr<const MapStyleValueDefinition> valueDefinition(pValueDefinition);
    _valuesDefinitions.insert(pValueDefinition->name, valueDefinition);
    return valueDefinition;
}

void OsmAnd::MapStyle_P::registerBuiltinValueDefinitions()
{
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
        registerBuiltinValueDefinition(_builtinValueDefs->varname);
#   include <MapStyleBuiltinValueDefinitions_Set.h>
#   undef DECLARE_BUILTIN_VALUEDEF
}

bool OsmAnd::MapStyle_P::resolveConstantValue( const QString& name, QString& value )
{
    auto itValue = _parsetimeConstants.constFind(name);
    if(itValue != _parsetimeConstants.cend())
    {
        value = *itValue;
        return true;
    }

    if(_parent)
        return _parent->_d->resolveConstantValue(name, value);
    return false;
}

QString OsmAnd::MapStyle_P::obtainValue( const QString& input )
{
    QString output;

    if(!input.isEmpty() && input[0] == '$')
    {
        if(!resolveConstantValue(input.mid(1), output))
            output = "unknown constant";
    }
    else
        output = input;

    return output;
}

QMap< uint64_t, std::shared_ptr<OsmAnd::MapStyleRule> >& OsmAnd::MapStyle_P::obtainRulesRef( MapStyleRulesetType type )
{
    switch (type)
    {
    case OsmAnd::MapStyleRulesetType::Point:
        return _pointRules;
    case OsmAnd::MapStyleRulesetType::Polyline:
        return _lineRules;
    case OsmAnd::MapStyleRulesetType::Polygon:
        return _polygonRules;
    case OsmAnd::MapStyleRulesetType::Text:
        return _textRules;
    case OsmAnd::MapStyleRulesetType::Order:
        return _orderRules;
    default:
        assert(false);
    }

    return *(QMap< uint64_t, std::shared_ptr<OsmAnd::MapStyleRule> >*)(nullptr);
}

const QMap< uint64_t, std::shared_ptr<OsmAnd::MapStyleRule> >& OsmAnd::MapStyle_P::obtainRulesRef( MapStyleRulesetType type ) const
{
    switch (type)
    {
    case OsmAnd::MapStyleRulesetType::Point:
        return _pointRules;
    case OsmAnd::MapStyleRulesetType::Polyline:
        return _lineRules;
    case OsmAnd::MapStyleRulesetType::Polygon:
        return _polygonRules;
    case OsmAnd::MapStyleRulesetType::Text:
        return _textRules;
    case OsmAnd::MapStyleRulesetType::Order:
        return _orderRules;
    default:
        assert(false);
    }

    return *(QMap< uint64_t, std::shared_ptr<OsmAnd::MapStyleRule> >*)(nullptr);
}

bool OsmAnd::MapStyle_P::registerRule( MapStyleRulesetType type, const std::shared_ptr<MapStyleRule>& rule )
{
    MapStyleValue tagData;
    if(!rule->getAttribute(_builtinValueDefs->INPUT_TAG, tagData))
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
        return false;
    }

    MapStyleValue valueData;
    if(!rule->getAttribute(_builtinValueDefs->INPUT_VALUE, valueData))
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
        return false;
    }

    uint64_t id = encodeRuleId(tagData.asSimple.asUInt, valueData.asSimple.asUInt);

    auto insertedRule = rule;
    auto& ruleset = obtainRulesRef(type);
    auto itPrevious = ruleset.constFind(id);
    if(itPrevious != ruleset.cend())
    {
        // all root rules should have at least tag/value
        insertedRule = createTagValueRootWrapperRule(id, *itPrevious);
        insertedRule->_d->_ifElseChildren.push_back(rule);
    }

    ruleset.insert(id, qMove(insertedRule));

    return true;
}

std::shared_ptr<OsmAnd::MapStyleRule> OsmAnd::MapStyle_P::createTagValueRootWrapperRule( uint64_t id, const std::shared_ptr<MapStyleRule>& rule )
{
    if(rule->_d->_values.size() <= 2)
        return rule;

    QHash< QString, QString > attributes;
    attributes.insert(QLatin1String("tag"), getTagString(id));
    attributes.insert(QLatin1String("value"), getValueString(id));
    std::shared_ptr<MapStyleRule> newRule(new MapStyleRule(owner, attributes));
    newRule->_d->_ifElseChildren.push_back(rule);
    return newRule;
}

uint32_t OsmAnd::MapStyle_P::getTagStringId( uint64_t ruleId ) const
{
    return ruleId >> RuleIdTagShift;
}

uint32_t OsmAnd::MapStyle_P::getValueStringId( uint64_t ruleId ) const
{
    return ruleId & ((1ull << RuleIdTagShift) - 1);
}

const QString& OsmAnd::MapStyle_P::getTagString( uint64_t ruleId ) const
{
    return lookupStringValue(getTagStringId(ruleId));
}

const QString& OsmAnd::MapStyle_P::getValueString( uint64_t ruleId ) const
{
    return lookupStringValue(getValueStringId(ruleId));
}

const QString& OsmAnd::MapStyle_P::lookupStringValue( uint32_t id ) const
{
    if(_parent && id < _stringsIdBase)
        return _parent->_d->lookupStringValue(id);
    return _stringsLUT[id - _stringsIdBase];
}

bool OsmAnd::MapStyle_P::lookupStringId( const QString& value, uint32_t& id ) const
{
    auto itId = _stringsRevLUT.constFind(value);
    if(itId != _stringsRevLUT.cend())
    {
        id = *itId;
        return true;
    }

    if(_parent)
        return _parent->_d->lookupStringId(value, id);

    return false;
}

uint32_t OsmAnd::MapStyle_P::lookupStringId( const QString& value )
{
    uint32_t id;
    if(lookupStringId(value, id))
        return id;

    return registerString(value);
}

uint32_t OsmAnd::MapStyle_P::registerString( const QString& value )
{
    const auto id = _stringsIdBase + _stringsLUT.size();

    _stringsRevLUT.insert(value, id);
    _stringsLUT.push_back(value);

    return id;
}

bool OsmAnd::MapStyle_P::mergeInherited()
{
    if(!_parent)
        return true;

    bool ok;

    ok = mergeInheritedAttributes();
    if(!ok)
        return false;

    ok = mergeInheritedRules(MapStyleRulesetType::Point);
    if(!ok)
        return false;

    ok = mergeInheritedRules(MapStyleRulesetType::Polyline);
    if(!ok)
        return false;

    ok = mergeInheritedRules(MapStyleRulesetType::Polygon);
    if(!ok)
        return false;

    ok = mergeInheritedRules(MapStyleRulesetType::Text);
    if(!ok)
        return false;

    ok = mergeInheritedRules(MapStyleRulesetType::Order);
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::MapStyle_P::mergeInheritedRules( MapStyleRulesetType type )
{
    if(!_parent)
        return true;

    const auto& parentRules = _parent->_d->obtainRulesRef(type);
    auto& rules = obtainRulesRef(type);

    for(const auto& parentRuleEntry : rangeOf(constOf(parentRules)))
    {
        auto itLocalRule = rules.constFind(parentRuleEntry.key());

        auto toInsert = parentRuleEntry.value();
        if(itLocalRule != rules.cend())
        {
            toInsert = createTagValueRootWrapperRule(parentRuleEntry.key(), *itLocalRule);
            toInsert->_d->_ifElseChildren.push_back(parentRuleEntry.value());
        }

        rules.insert(parentRuleEntry.key(), toInsert);
    }

    return true;
}

bool OsmAnd::MapStyle_P::mergeInheritedAttributes()
{
    if(!_parent)
        return true;

    const auto& itEnd = _attributes.cend();
    for(const auto& parentAttributeEntry : rangeOf(constOf(_parent->_d->_attributes)))
    {
        const auto& parentAttribute = parentAttributeEntry.value();

        const auto& itAttribute = _attributes.constFind(parentAttributeEntry.key());
        if(itAttribute != itEnd)
        {
            const auto& attribute = *itAttribute;
            for(const auto& child : constOf(parentAttribute->_d->_ifElseChildren))
                attribute->_d->_ifElseChildren.push_back(child);
        }
        else
        {
            _attributes.insert(parentAttributeEntry.key(), parentAttribute);
        }
    }

    return true;
}

uint64_t OsmAnd::MapStyle_P::encodeRuleId( uint32_t tag, uint32_t value )
{
    return (static_cast<uint64_t>(tag) << RuleIdTagShift) | value;
}
