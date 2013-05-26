#include "RasterizationStyle.h"

#include <cassert>
#include <iostream>

#include <QByteArray>
#include <QBuffer>
#include <QFileInfo>
#include <QStack>

#include "Logging.h"
#include "RasterizationStyles.h"
#include "RasterizationRule.h"
#include "EmbeddedResources.h"

OsmAnd::RasterizationStyle::RasterizationStyle( RasterizationStyles* owner, const QString& embeddedResourceName )
    : owner(owner)
    , _firstNonBuiltinValueDefinitionIndex(0)
    , _stringsIdBase(0)
    , _resourceName(embeddedResourceName)
    , resourceName(_resourceName)
    , externalFileName(_externalFileName)
    , title(_title)
    , name(_name)
    , parentName(_parentName)
{
    _name = QFileInfo(embeddedResourceName).fileName().replace(".render.xml", "");
    registerBuiltinValueDefinitions();
}

OsmAnd::RasterizationStyle::RasterizationStyle( RasterizationStyles* owner, const QFile& externalStyleFile )
    : owner(owner)
    , _firstNonBuiltinValueDefinitionIndex(0)
    , _stringsIdBase(0)
    , resourceName(_resourceName)
    , _externalFileName(externalStyleFile.fileName())
    , externalFileName(_externalFileName)
    , title(_title)
    , name(_name)
    , parentName(_parentName)
{
    _name = QFileInfo(externalStyleFile.fileName()).fileName().replace(".render.xml", "");
    registerBuiltinValueDefinitions();
}

OsmAnd::RasterizationStyle::~RasterizationStyle()
{
}

bool OsmAnd::RasterizationStyle::parseMetadata()
{
    if(!_resourceName.isEmpty())
    {
        QXmlStreamReader data(EmbeddedResources::decompressResource(_resourceName));
        return parseMetadata(data);
    }
    else if(!_externalFileName.isEmpty())
    {
        QFile styleFile(_externalFileName);
        if(!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QXmlStreamReader data(&styleFile);
        bool ok = parseMetadata(data);
        styleFile.close();
        return ok;
    }

    return false;
}

bool OsmAnd::RasterizationStyle::parseMetadata( QXmlStreamReader& xmlReader )
{
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == "renderingStyle")
            {
                _title = xmlReader.attributes().value("name").toString();
                auto attrDepends = xmlReader.attributes().value("depends");
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

bool OsmAnd::RasterizationStyle::isStandalone() const
{
    return _parentName.isEmpty();
}

bool OsmAnd::RasterizationStyle::areDependenciesResolved() const
{
    if(isStandalone())
        return true;

    return _parent && _parent->areDependenciesResolved();
}

bool OsmAnd::RasterizationStyle::resolveDependencies()
{
    if(_parentName.isEmpty())
        return true;

    // Make sure parent is resolved before this style (if present)
    if(!_parentName.isEmpty() && !_parent)
    {
        if(!owner->obtainStyle(_parentName, _parent))
            return false;
    }

    // Obtain string ID base
    _stringsIdBase = _parent->_stringsIdBase + _parent->_stringsLUT.size();

    return true;
}

bool OsmAnd::RasterizationStyle::parse()
{
    if(!_resourceName.isEmpty())
    {
        QXmlStreamReader data(EmbeddedResources::decompressResource(_resourceName));
        return parse(data);
    }
    else if(!_externalFileName.isEmpty())
    {
        QFile styleFile(_externalFileName);
        if(!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QXmlStreamReader data(&styleFile);
        bool ok = parse(data);
        styleFile.close();
        return ok;
    }

    return false;
}

bool OsmAnd::RasterizationStyle::parse( QXmlStreamReader& xmlReader )
{
    auto rulesetType = RulesetType::Invalid;

    struct Lexeme
    {
        enum Type
        {
            Rule,
            Group,
        };

        Lexeme(Type type_, RasterizationStyle* style_)
            : type(type_)
            , style(style_)
        {}

        const Type type;
        RasterizationStyle* const style;
    };

    struct Rule : public Lexeme
    {
        Rule(std::shared_ptr<RasterizationRule> rule_, RasterizationStyle* style_)
            : Lexeme(Lexeme::Rule, style_)
            , rule(rule_)
        {}

        const std::shared_ptr<RasterizationRule> rule;
    };

    struct Group : public Lexeme
    {
        Group(RasterizationStyle* style_)
            : Lexeme(Lexeme::Group, style_)
        {}

        QHash< QString, QString > attributes;
        QList< std::shared_ptr<RasterizationRule> > children;
        QList< std::shared_ptr<Lexeme> > subgroups;

        void addGroupFilter(std::shared_ptr<RasterizationRule> rule)
        {
            for(auto itChild = children.begin(); itChild != children.end(); ++itChild)
            {
                auto child = *itChild;

                child->_ifChildren.push_back(rule);
            }

            for(auto itSubgroup = subgroups.begin(); itSubgroup != subgroups.end(); ++itSubgroup)
            {
                auto subgroup = *itSubgroup;

                assert(subgroup->type == Lexeme::Group);
                static_cast<Group*>(subgroup.get())->addGroupFilter(rule);
            }
        }

        bool registerGlobalRules(RulesetType type)
        {
            for(auto itChild = children.begin(); itChild != children.end(); ++itChild)
            {
                auto child = *itChild;

                if(!style->registerRule(type, child))
                    return false;
            }

            for(auto itSubgroup = subgroups.begin(); itSubgroup != subgroups.end(); ++itSubgroup)
            {
                auto subgroup = *itSubgroup;

                assert(subgroup->type == Lexeme::Group);
                if(!static_cast<Group*>(subgroup.get())->registerGlobalRules(type))
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
            if (tagName == "renderingStyle")
            {
                // We have already parsed metadata and resolved dependencies, so here we don't need to do anything
            }
            else if(tagName == "renderingConstant")
            {
                const auto& attribs = xmlReader.attributes();

                auto name = obtainValue(attribs.value("name").toString());
                auto value = obtainValue(attribs.value("value").toString());
                _parsetimeConstants.insert(name, value);
            }
            else if(tagName == "renderingProperty")
            {
                ConfigurableInputValue* inputValue = nullptr;

                const auto& attribs = xmlReader.attributes();
                auto name = obtainValue(attribs.value("attr").toString());
                auto type = obtainValue(attribs.value("type"));
                auto title = obtainValue(attribs.value("name").toString());
                auto description = attribs.value("description").toString();
                auto encodedPossibleValues = attribs.value("possibleValues");
                QStringList possibleValues;
                if(!encodedPossibleValues.isEmpty())
                    possibleValues = encodedPossibleValues.toString().split(",", QString::SkipEmptyParts);
                for(auto itPossibleValue = possibleValues.begin(); itPossibleValue != possibleValues.end(); ++itPossibleValue)
                {
                    auto& possibleValue = *itPossibleValue;
                    possibleValue = obtainValue(possibleValue);
                }
                if(type == "string")
                {
                    inputValue = new ConfigurableInputValue(
                        ValueDefinition::String,
                        name,
                        title,
                        description,
                        possibleValues);
                }
                else if(type == "boolean")
                {
                    inputValue = new ConfigurableInputValue(
                        ValueDefinition::Boolean,
                        name,
                        title,
                        description,
                        possibleValues);
                }
                else // treat as integer
                {
                    inputValue = new ConfigurableInputValue(
                        ValueDefinition::Integer,
                        name,
                        title,
                        description,
                        possibleValues);
                }

                registerValue(inputValue);
            }
            else if(tagName == "renderingAttribute")
            {
                const auto& attribs = xmlReader.attributes();

                auto name = obtainValue(attribs.value("name").toString());

                std::shared_ptr<RasterizationRule> attribute(new RasterizationRule(this, QHash< QString, QString >()));
                //TODO:t->storage->renderingAttributes[name] = attribute;

                std::shared_ptr<Lexeme> selector(new Rule(attribute, this));
                stack.push(selector);
            }
            else if(tagName == "filter")
            {
                QHash< QString, QString > ruleAttributes;

                if(!stack.isEmpty())
                {
                    auto lexeme = stack.top();

                    if(lexeme->type == Lexeme::Group)
                        ruleAttributes.unite(static_cast<Group*>(lexeme.get())->attributes);
                }

                const auto& attribs = xmlReader.attributes();
                for(auto itXmlAttrib = attribs.begin(); itXmlAttrib != attribs.end(); ++itXmlAttrib)
                {
                    const auto& xmlAttrib = *itXmlAttrib;

                    auto tag = xmlAttrib.name().toString();
                    auto value = obtainValue(xmlAttrib.value());
                    ruleAttributes.insert(tag, value);
                }

                std::shared_ptr<RasterizationRule> rule(new RasterizationRule(this, ruleAttributes));

                if(!stack.isEmpty())
                {
                    auto lexeme = stack.top();

                    if(lexeme->type == Lexeme::Group)
                    {
                        static_cast<Group*>(lexeme.get())->children.push_back(rule);
                    }
                    else if(lexeme->type == Lexeme::Rule)
                    {
                        static_cast<Rule*>(lexeme.get())->rule->_ifElseChildren.push_back(rule);
                    }
                }
                else
                {
                    if(!registerRule(rulesetType, rule))
                        return false;
                }

                stack.push(std::shared_ptr<Lexeme>(new Rule(rule, this)));
            }
            else if(tagName == "groupFilter")
            {
                QHash< QString, QString > attributes;

                const auto& attribs = xmlReader.attributes();
                for(auto itXmlAttrib = attribs.begin(); itXmlAttrib != attribs.end(); ++itXmlAttrib)
                {
                    const auto& xmlAttrib = *itXmlAttrib;

                    auto tag = xmlAttrib.name().toString();
                    auto value = obtainValue(xmlAttrib.value());
                    attributes.insert(tag, value);
                }

                std::shared_ptr<RasterizationRule> rule(new RasterizationRule(this, attributes));
                
                if(stack.isEmpty())
                {
                    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Group filter without parent");
                    return false;
                }

                auto lexeme = stack.top();
                if(lexeme->type == Lexeme::Group)
                {
                    static_cast<Group*>(lexeme.get())->addGroupFilter(rule);
                }
                else if(lexeme->type == Lexeme::Rule)
                {
                    static_cast<Rule*>(lexeme.get())->rule->_ifChildren.push_back(rule);
                }
                
                stack.push(std::shared_ptr<Lexeme>(new Rule(rule, this)));
            }
            else if(tagName == "group")
            {
                auto group = new Group(this);
                if(!stack.isEmpty())
                {
                    auto lexeme = stack.top();
                    if(lexeme->type == Lexeme::Group)
                        group->attributes.unite(static_cast<Group*>(lexeme.get())->attributes);
                }
                
                const auto& attribs = xmlReader.attributes();
                for(auto itXmlAttrib = attribs.begin(); itXmlAttrib != attribs.end(); ++itXmlAttrib)
                {
                    const auto& xmlAttrib = *itXmlAttrib;

                    auto tag = xmlAttrib.name().toString();
                    auto value = obtainValue(xmlAttrib.value());
                    group->attributes.insert(tag, value);
                }

                stack.push(std::shared_ptr<Lexeme>(group));
            }
            else if(tagName == "order")
            {
                rulesetType = RulesetType::Order;
            }
            else if(tagName == "text")
            {
                rulesetType = RulesetType::Text;
            }
            else if(tagName == "point")
            {
                rulesetType = RulesetType::Point;
            }
            else if(tagName == "line")
            {
                rulesetType = RulesetType::Line;
            }
            else if(tagName == "polygon")
            {
                rulesetType = RulesetType::Polygon;
            } 
        }
        else if(xmlReader.isEndElement())
        {
            if(tagName == "filter")
            {
                stack.pop();
            }
            else if(tagName == "group")
            {
                auto lexeme = stack.pop();

                if (stack.isEmpty())
                {
                    assert(lexeme->type == Lexeme::Group);
                    if(!static_cast<Group*>(lexeme.get())->registerGlobalRules(rulesetType))
                        return false;
                }
                else
                {
                    auto group = stack.top();
                    if(group->type == Lexeme::Group)
                        static_cast<Group*>(group.get())->subgroups.push_back(lexeme);
                }
            }
            else if(tagName == "groupFilter")
            {
                stack.pop();
            }
            else if(tagName == "renderingAttribute")
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

void OsmAnd::RasterizationStyle::registerBuiltinValue( const std::shared_ptr<ValueDefinition>& valueDefinition )
{
    assert(_valuesDefinitions.size() == 0 || _valuesDefinitions.size() <= _firstNonBuiltinValueDefinitionIndex);

    _valuesDefinitions.insert(valueDefinition->name, valueDefinition);
    _firstNonBuiltinValueDefinitionIndex = _valuesDefinitions.size();
}

std::shared_ptr<OsmAnd::RasterizationStyle::ValueDefinition> OsmAnd::RasterizationStyle::registerValue( ValueDefinition* pValueDefinition )
{
    std::shared_ptr<ValueDefinition> valueDefinition(pValueDefinition);
    _valuesDefinitions.insert(pValueDefinition->name, valueDefinition);
    return valueDefinition;
}

void OsmAnd::RasterizationStyle::registerBuiltinValueDefinitions()
{
    registerBuiltinValue(builtinValueDefinitions.INPUT_TEST);
    registerBuiltinValue(builtinValueDefinitions.INPUT_TEXT_LENGTH);
    registerBuiltinValue(builtinValueDefinitions.INPUT_TAG);
    registerBuiltinValue(builtinValueDefinitions.INPUT_VALUE);
    registerBuiltinValue(builtinValueDefinitions.INPUT_MINZOOM);
    registerBuiltinValue(builtinValueDefinitions.INPUT_MAXZOOM);
    registerBuiltinValue(builtinValueDefinitions.INPUT_NIGHT_MODE);
    registerBuiltinValue(builtinValueDefinitions.INPUT_LAYER);
    registerBuiltinValue(builtinValueDefinitions.INPUT_POINT);
    registerBuiltinValue(builtinValueDefinitions.INPUT_AREA);
    registerBuiltinValue(builtinValueDefinitions.INPUT_CYCLE);
    registerBuiltinValue(builtinValueDefinitions.INPUT_NAME_TAG);
    registerBuiltinValue(builtinValueDefinitions.INPUT_ADDITIONAL);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_SHIELD);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_SHADOW_RADIUS);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_SHADOW_COLOR);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_SHADER);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_CAP_3);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_CAP_2);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_CAP);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_CAP_0);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_CAP__1);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_PATH_EFFECT_3);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_PATH_EFFECT_2);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_PATH_EFFECT);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_PATH_EFFECT_0);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_PATH_EFFECT__1);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_STROKE_WIDTH_3);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_STROKE_WIDTH_2);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_STROKE_WIDTH);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_STROKE_WIDTH_0);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_STROKE_WIDTH__1);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_COLOR_3);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_COLOR);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_COLOR_2);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_COLOR_0);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_COLOR__1);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_BOLD);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_ORDER);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_MIN_DISTANCE);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_ON_PATH);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_ICON);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_ORDER);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_SHADOW_LEVEL);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_DY);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_SIZE);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_COLOR);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_HALO_RADIUS);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_TEXT_WRAP_WIDTH);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_OBJECT_TYPE);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_ATTR_COLOR_VALUE);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_ATTR_BOOL_VALUE);
    registerBuiltinValue(builtinValueDefinitions.OUTPUT_ATTR_STRING_VALUE);
}

bool OsmAnd::RasterizationStyle::resolveConstantValue( const QString& name, QString& value )
{
    auto itValue = _parsetimeConstants.find(name);
    if(itValue != _parsetimeConstants.end())
    {
        value = *itValue;
        return true;
    }

    if(_parent)
        return _parent->resolveConstantValue(name, value);
    return false;
}

QString OsmAnd::RasterizationStyle::obtainValue( const QString& input )
{
    QString output;

    if(input[0] == '$')
    {
        if(!resolveConstantValue(input.mid(1), output))
            output = "unknown constant";
    }
    else
        output = input;

    return output;
}

QString OsmAnd::RasterizationStyle::obtainValue( const QStringRef& input )
{
    QString output;

    if(input.string()->at(0) == '$')
    {
        if(!resolveConstantValue(input.toString().mid(1), output))
            output = "unknown constant";
    }
    else
        output = input.toString();

    return output;
}

QMap< uint64_t, std::shared_ptr<OsmAnd::RasterizationRule> >& OsmAnd::RasterizationStyle::obtainRules( RulesetType type )
{
    switch (type)
    {
    case OsmAnd::RasterizationStyle::Point:
        return _pointRules;
    case OsmAnd::RasterizationStyle::Line:
        return _lineRules;
    case OsmAnd::RasterizationStyle::Polygon:
        return _polygonRules;
    case OsmAnd::RasterizationStyle::Text:
        return _textRules;
    case OsmAnd::RasterizationStyle::Order:
        return _orderRules;
    }
    
    assert(type >= Point && type <= Order);
    return *(QMap< uint64_t, std::shared_ptr<OsmAnd::RasterizationRule> >*)(nullptr);
}

const QMap< uint64_t, std::shared_ptr<OsmAnd::RasterizationRule> >& OsmAnd::RasterizationStyle::obtainRules( RulesetType type ) const
{
    switch (type)
    {
    case OsmAnd::RasterizationStyle::Point:
        return _pointRules;
    case OsmAnd::RasterizationStyle::Line:
        return _lineRules;
    case OsmAnd::RasterizationStyle::Polygon:
        return _polygonRules;
    case OsmAnd::RasterizationStyle::Text:
        return _textRules;
    case OsmAnd::RasterizationStyle::Order:
        return _orderRules;
    }

    assert(type >= Point && type <= Order);
    return *(QMap< uint64_t, std::shared_ptr<OsmAnd::RasterizationRule> >*)(nullptr);
}

void OsmAnd::RasterizationStyle::dump( const QString& prefix /*= QString()*/ ) const
{
    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sPoint rules:\n", prefix.toStdString().c_str());
    dump(RulesetType::Point, prefix);

    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sLine rules:\n", prefix.toStdString().c_str());
    dump(RulesetType::Line, prefix);

    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sPolygon rules:\n", prefix.toStdString().c_str());
    dump(RulesetType::Polygon, prefix);

    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sText rules:\n", prefix.toStdString().c_str());
    dump(RulesetType::Text, prefix);

    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sOrder rules:\n", prefix.toStdString().c_str());
    dump(RulesetType::Order, prefix);
}

void OsmAnd::RasterizationStyle::dump( RulesetType type, const QString& prefix /*= QString()*/ ) const
{
    const auto& rules = obtainRules(type);
    for(auto itRuleEntry = rules.begin(); itRuleEntry != rules.end(); ++itRuleEntry)
    {
        auto tag = getTagString(itRuleEntry.key());
        auto value = getValueString(itRuleEntry.key());
        auto rule = itRuleEntry.value();

        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%sRule [%s:%s]\n", prefix.toStdString().c_str(), tag.toStdString().c_str(), value.toStdString().c_str());
        rule->dump(prefix);
    }
}

bool OsmAnd::RasterizationStyle::registerRule( RulesetType type, const std::shared_ptr<RasterizationRule>& rule )
{
    bool ok;

    auto tag = rule->getIntegerAttribute(RasterizationStyle::builtinValueDefinitions.INPUT_TAG->name, &ok);
    if(!ok)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
        return false;
    }

    auto value = rule->getIntegerAttribute(RasterizationStyle::builtinValueDefinitions.INPUT_VALUE->name, &ok);
    if(!ok)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
        return false;
    }

    uint64_t id = encodeRuleId(tag, value);

    auto insertedRule = rule;
    auto& ruleset = obtainRules(type);
    auto itPrevious = ruleset.find(id);
    if(itPrevious != ruleset.end())
    {
        // all root rules should have at least tag/value
        insertedRule = createTagValueRootWrapperRule(id, *itPrevious);
        insertedRule->_ifElseChildren.push_back(rule);
    }
    
    ruleset.insert(id, insertedRule);

    return true;
}

std::shared_ptr<OsmAnd::RasterizationRule> OsmAnd::RasterizationStyle::createTagValueRootWrapperRule( uint64_t id, const std::shared_ptr<RasterizationRule>& rule )
{
    if(rule->_valueDefinitionsRefs.size() <= 2)
        return rule;

    QHash< QString, QString > attributes;
    attributes.insert("tag", getTagString(id));
    attributes.insert("value", getValueString(id));
    std::shared_ptr<RasterizationRule> newRule(new RasterizationRule(this, attributes));
    newRule->_ifElseChildren.push_back(rule);
    return newRule;
}

const QString& OsmAnd::RasterizationStyle::getTagString( uint64_t ruleId ) const
{
    return lookupStringValue(ruleId >> RuleIdTagShift);
}

const QString& OsmAnd::RasterizationStyle::getValueString( uint64_t ruleId ) const
{
    return lookupStringValue(ruleId & ((1ull << RuleIdTagShift) - 1));
}

const QString& OsmAnd::RasterizationStyle::lookupStringValue( uint32_t id ) const
{
    if(_parent && id < _stringsIdBase)
        return _parent->lookupStringValue(id);
    return _stringsLUT[id - _stringsIdBase];
}

bool OsmAnd::RasterizationStyle::lookupStringId( const QString& value, uint32_t& id )
{
    auto itId = _stringsRevLUT.find(value);
    if(itId != _stringsRevLUT.end())
    {
        id = *itId;
        return true;
    }

    if(_parent)
        return _parent->lookupStringId(value, id);

    return false;
}

uint32_t OsmAnd::RasterizationStyle::lookupStringId( const QString& value )
{
    uint32_t id;
    if(lookupStringId(value, id))
        return id;
    
    return registerString(value);
}

uint32_t OsmAnd::RasterizationStyle::registerString( const QString& value )
{
    auto id = _stringsIdBase + _stringsLUT.size();
    
    _stringsRevLUT.insert(value, id);
    _stringsLUT.push_back(value);

    return id;
}

bool OsmAnd::RasterizationStyle::resolveValueDefinition( const QString& name, std::shared_ptr<ValueDefinition>& outDefinition )
{
    auto itValueDefinition = _valuesDefinitions.find(name);
    if(itValueDefinition != _valuesDefinitions.end())
    {
        outDefinition = *itValueDefinition;
        return true;
    }

    if(!_parent)
        return false;

    return _parent->resolveValueDefinition(name, outDefinition);
}

bool OsmAnd::RasterizationStyle::mergeInherited()
{
    if(!_parent)
        return true;

    bool ok;

    // merge results
    // dictionary and props are already merged
    /*map<std::string,  RenderingRule*>::iterator it = depends->renderingAttributes.begin();
    for(;it != depends->renderingAttributes.end(); it++) {
        map<std::string,  RenderingRule*>::iterator o = renderingAttributes.find(it->first);
        if (o != renderingAttributes.end()) {
            std::vector<RenderingRule*>::iterator list = it->second->ifElseChildren.begin();
            for (;list != it->second->ifElseChildren.end(); list++) {
                o->second->ifElseChildren.push_back(*list);
            }
        } else {
            renderingAttributes[it->first] = it->second;
        }
    }*/

    ok = mergeInheritedRules(RulesetType::Point);
    if(!ok)
        return false;

    ok = mergeInheritedRules(RulesetType::Line);
    if(!ok)
        return false;

    ok = mergeInheritedRules(RulesetType::Polygon);
    if(!ok)
        return false;

    ok = mergeInheritedRules(RulesetType::Text);
    if(!ok)
        return false;

    ok = mergeInheritedRules(RulesetType::Order);
    if(!ok)
        return false;

    return true;
}

bool OsmAnd::RasterizationStyle::mergeInheritedRules( RulesetType type )
{
    if(!_parent)
        return true;

    const auto& parentRules = _parent->obtainRules(type);
    auto& rules = obtainRules(type);

    for(auto itParentRule = parentRules.begin(); itParentRule != parentRules.end(); ++itParentRule)
    {
        auto itLocalRule = rules.find(itParentRule.key());

        auto toInsert = itParentRule.value();
        if(itLocalRule != rules.end())
        {
            toInsert = createTagValueRootWrapperRule(itParentRule.key(), *itLocalRule);
            toInsert->_ifElseChildren.push_back(itParentRule.value());
        }

        rules.insert(itParentRule.key(), toInsert);
    }

    return true;
}

uint64_t OsmAnd::RasterizationStyle::encodeRuleId( uint32_t tag, uint32_t value )
{
    return (static_cast<uint64_t>(tag) << RuleIdTagShift) | value;
}

OsmAnd::RasterizationStyle::ValueDefinition::ValueDefinition( ValueDefinition::Type type_, ValueDefinition::DataType dataType_, const QString& name_ )
    : type(type_)
    , dataType(dataType_)
    , name(name_)
{
}

OsmAnd::RasterizationStyle::ValueDefinition::~ValueDefinition()
{
}

OsmAnd::RasterizationStyle::ConfigurableInputValue::ConfigurableInputValue( ValueDefinition::DataType type_, const QString& name_, const QString& title_, const QString& description_, const QStringList& possibleValues_ )
    : ValueDefinition(Input, type_, name_)
    , title(title_)
    , description(description_)
    , possibleValues(possibleValues_)
{
}

OsmAnd::RasterizationStyle::ConfigurableInputValue::~ConfigurableInputValue()
{
}

const OsmAnd::RasterizationStyle::_builtinValueDefinitions OsmAnd::RasterizationStyle::builtinValueDefinitions;

#define DECLARE_BUILTIN_VALUEDEF(varname, type, dataType, name) \
    varname(new OsmAnd::RasterizationStyle::ValueDefinition( \
        OsmAnd::RasterizationStyle::ValueDefinition::##type##, \
        OsmAnd::RasterizationStyle::ValueDefinition::##dataType##, \
        name \
    ))

OsmAnd::RasterizationStyle::_builtinValueDefinitions::_builtinValueDefinitions()
    : DECLARE_BUILTIN_VALUEDEF(INPUT_TEST, Input, Boolean, "test")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_TAG, Input, String, "tag")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_VALUE, Input, String, "value")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_ADDITIONAL, Input, String, "additional")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_MINZOOM, Input, Integer, "minzoom")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_MAXZOOM, Input, Integer, "maxzoom")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_NIGHT_MODE, Input, Boolean, "nightMode")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_LAYER, Input, Integer, "layer")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_POINT, Input, Boolean, "point")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_AREA, Input, Boolean, "area")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_CYCLE, Input, Boolean, "cycle")

    , DECLARE_BUILTIN_VALUEDEF(INPUT_TEXT_LENGTH, Input, Integer, "textLength")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_NAME_TAG, Input, String, "nameTag")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_INT_VALUE, Output, Integer, "attrIntValue")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_BOOL_VALUE, Output, Boolean, "attrBoolValue")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_COLOR_VALUE, Output, Color, "attrColorValue")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_STRING_VALUE, Output, String, "attrStringValue")

    // order - no sense to make it float
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ORDER, Output, Integer, "order")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_OBJECT_TYPE, Output, Integer, "objectType")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_LEVEL, Output, Integer, "shadowLevel")

    // text properties
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_WRAP_WIDTH, Output, Integer, "textWrapWidth")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_DY, Output, Integer, "textDy")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_HALO_RADIUS, Output, Integer, "textHaloRadius")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SIZE, Output, Integer, "textSize")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ORDER, Output, Integer, "textOrder")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_MIN_DISTANCE, Output, Integer, "textMinDistance")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SHIELD, Output, String, "textShield")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_COLOR, Output, Color, "textColor")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_BOLD, Output, Boolean, "textBold")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ON_PATH, Output, Boolean, "textOnPath")

    // point
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON, Output, String, "icon")

    // polygon/way
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR, Output, Color, "color")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_2, Output, Color, "color_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_3, Output, Color, "color_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_0, Output, Color, "color_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR__1, Output, Color, "color__1")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH, Output, Float, "strokeWidth")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_2, Output, Float, "strokeWidth_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_3, Output, Float, "strokeWidth_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_0, Output, Float, "strokeWidth_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH__1, Output, Float, "strokeWidth__1")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT, Output, String, "pathEffect")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_2, Output, String, "pathEffect_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_3, Output, String, "pathEffect_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_0, Output, String, "pathEffect_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT__1, Output, String, "pathEffect__1")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP, Output, String, "cap")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_2, Output, String, "cap_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_3, Output, String, "cap_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_0, Output, String, "cap_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP__1, Output, String, "cap__1")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADER, Output, String, "shader")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_COLOR, Output, Color, "shadowColor")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_RADIUS, Output, Integer, "shadowRadius")
{
}
#undef DECLARE_BUILTIN_VALUEDEF