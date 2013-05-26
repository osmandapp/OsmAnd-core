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

//TODO: Once Visual Studio C++ compiler will support C++11 delegation ctors, this should be cleaned-up

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
    , INPUT_TEST(_builtin_INPUT_TEST)
    , INPUT_TEXT_LENGTH(_builtin_INPUT_TEXT_LENGTH)
    , INPUT_TAG(_builtin_INPUT_TAG)
    , INPUT_VALUE(_builtin_INPUT_VALUE)
    , INPUT_MINZOOM(_builtin_INPUT_MINZOOM)
    , INPUT_MAXZOOM(_builtin_INPUT_MAXZOOM)
    , INPUT_NIGHT_MODE(_builtin_INPUT_NIGHT_MODE)
    , INPUT_LAYER(_builtin_INPUT_LAYER)
    , INPUT_POINT(_builtin_INPUT_POINT)
    , INPUT_AREA(_builtin_INPUT_AREA)
    , INPUT_CYCLE(_builtin_INPUT_CYCLE)
    , INPUT_NAME_TAG(_builtin_INPUT_NAME_TAG)
    , INPUT_ADDITIONAL(_builtin_INPUT_ADDITIONAL)
    , OUTPUT_REF(_builtin_OUTPUT_REF)
    , OUTPUT_TEXT_SHIELD(_builtin_OUTPUT_TEXT_SHIELD)
    , OUTPUT_SHADOW_RADIUS(_builtin_OUTPUT_SHADOW_RADIUS)
    , OUTPUT_SHADOW_COLOR(_builtin_OUTPUT_SHADOW_COLOR)
    , OUTPUT_SHADER(_builtin_OUTPUT_SHADER)
    , OUTPUT_CAP_3(_builtin_OUTPUT_CAP_3)
    , OUTPUT_CAP_2(_builtin_OUTPUT_CAP_2)
    , OUTPUT_CAP(_builtin_OUTPUT_CAP)
    , OUTPUT_CAP_0(_builtin_OUTPUT_CAP_0)
    , OUTPUT_CAP__1(_builtin_OUTPUT_CAP__1)
    , OUTPUT_PATH_EFFECT_3(_builtin_OUTPUT_PATH_EFFECT_3)
    , OUTPUT_PATH_EFFECT_2(_builtin_OUTPUT_PATH_EFFECT_2)
    , OUTPUT_PATH_EFFECT(_builtin_OUTPUT_PATH_EFFECT)
    , OUTPUT_PATH_EFFECT_0(_builtin_OUTPUT_PATH_EFFECT_0)
    , OUTPUT_PATH_EFFECT__1(_builtin_OUTPUT_PATH_EFFECT__1)
    , OUTPUT_STROKE_WIDTH_3(_builtin_OUTPUT_STROKE_WIDTH_3)
    , OUTPUT_STROKE_WIDTH_2(_builtin_OUTPUT_STROKE_WIDTH_2)
    , OUTPUT_STROKE_WIDTH(_builtin_OUTPUT_STROKE_WIDTH)
    , OUTPUT_STROKE_WIDTH_0(_builtin_OUTPUT_STROKE_WIDTH_0)
    , OUTPUT_STROKE_WIDTH__1(_builtin_OUTPUT_STROKE_WIDTH__1)
    , OUTPUT_COLOR_3(_builtin_OUTPUT_COLOR_3)
    , OUTPUT_COLOR(_builtin_OUTPUT_COLOR)
    , OUTPUT_COLOR_2(_builtin_OUTPUT_COLOR_2)
    , OUTPUT_COLOR_0(_builtin_OUTPUT_COLOR_0)
    , OUTPUT_COLOR__1(_builtin_OUTPUT_COLOR__1)
    , OUTPUT_TEXT_BOLD(_builtin_OUTPUT_TEXT_BOLD)
    , OUTPUT_TEXT_ORDER(_builtin_OUTPUT_TEXT_ORDER)
    , OUTPUT_TEXT_MIN_DISTANCE(_builtin_OUTPUT_TEXT_MIN_DISTANCE)
    , OUTPUT_TEXT_ON_PATH(_builtin_OUTPUT_TEXT_ON_PATH)
    , OUTPUT_ICON(_builtin_OUTPUT_ICON)
    , OUTPUT_ORDER(_builtin_OUTPUT_ORDER)
    , OUTPUT_SHADOW_LEVEL(_builtin_OUTPUT_SHADOW_LEVEL)
    , OUTPUT_TEXT_DY(_builtin_OUTPUT_TEXT_DY)
    , OUTPUT_TEXT_SIZE(_builtin_OUTPUT_TEXT_SIZE)
    , OUTPUT_TEXT_COLOR(_builtin_OUTPUT_TEXT_COLOR)
    , OUTPUT_TEXT_HALO_RADIUS(_builtin_OUTPUT_TEXT_HALO_RADIUS)
    , OUTPUT_TEXT_WRAP_WIDTH(_builtin_OUTPUT_TEXT_WRAP_WIDTH)
    , OUTPUT_OBJECT_TYPE(_builtin_OUTPUT_OBJECT_TYPE)
    , OUTPUT_ATTR_INT_VALUE(_builtin_OUTPUT_ATTR_INT_VALUE)
    , OUTPUT_ATTR_COLOR_VALUE(_builtin_OUTPUT_ATTR_COLOR_VALUE)
    , OUTPUT_ATTR_BOOL_VALUE(_builtin_OUTPUT_ATTR_BOOL_VALUE)
    , OUTPUT_ATTR_STRING_VALUE(_builtin_OUTPUT_ATTR_STRING_VALUE)
{
    _name = QFileInfo(embeddedResourceName).fileName().replace(".render.xml", "");
    inflateBuiltinValues();
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
    , INPUT_TEST(_builtin_INPUT_TEST)
    , INPUT_TEXT_LENGTH(_builtin_INPUT_TEXT_LENGTH)
    , INPUT_TAG(_builtin_INPUT_TAG)
    , INPUT_VALUE(_builtin_INPUT_VALUE)
    , INPUT_MINZOOM(_builtin_INPUT_MINZOOM)
    , INPUT_MAXZOOM(_builtin_INPUT_MAXZOOM)
    , INPUT_NIGHT_MODE(_builtin_INPUT_NIGHT_MODE)
    , INPUT_LAYER(_builtin_INPUT_LAYER)
    , INPUT_POINT(_builtin_INPUT_POINT)
    , INPUT_AREA(_builtin_INPUT_AREA)
    , INPUT_CYCLE(_builtin_INPUT_CYCLE)
    , INPUT_NAME_TAG(_builtin_INPUT_NAME_TAG)
    , INPUT_ADDITIONAL(_builtin_INPUT_ADDITIONAL)
    , OUTPUT_REF(_builtin_OUTPUT_REF)
    , OUTPUT_TEXT_SHIELD(_builtin_OUTPUT_TEXT_SHIELD)
    , OUTPUT_SHADOW_RADIUS(_builtin_OUTPUT_SHADOW_RADIUS)
    , OUTPUT_SHADOW_COLOR(_builtin_OUTPUT_SHADOW_COLOR)
    , OUTPUT_SHADER(_builtin_OUTPUT_SHADER)
    , OUTPUT_CAP_3(_builtin_OUTPUT_CAP_3)
    , OUTPUT_CAP_2(_builtin_OUTPUT_CAP_2)
    , OUTPUT_CAP(_builtin_OUTPUT_CAP)
    , OUTPUT_CAP_0(_builtin_OUTPUT_CAP_0)
    , OUTPUT_CAP__1(_builtin_OUTPUT_CAP__1)
    , OUTPUT_PATH_EFFECT_3(_builtin_OUTPUT_PATH_EFFECT_3)
    , OUTPUT_PATH_EFFECT_2(_builtin_OUTPUT_PATH_EFFECT_2)
    , OUTPUT_PATH_EFFECT(_builtin_OUTPUT_PATH_EFFECT)
    , OUTPUT_PATH_EFFECT_0(_builtin_OUTPUT_PATH_EFFECT_0)
    , OUTPUT_PATH_EFFECT__1(_builtin_OUTPUT_PATH_EFFECT__1)
    , OUTPUT_STROKE_WIDTH_3(_builtin_OUTPUT_STROKE_WIDTH_3)
    , OUTPUT_STROKE_WIDTH_2(_builtin_OUTPUT_STROKE_WIDTH_2)
    , OUTPUT_STROKE_WIDTH(_builtin_OUTPUT_STROKE_WIDTH)
    , OUTPUT_STROKE_WIDTH_0(_builtin_OUTPUT_STROKE_WIDTH_0)
    , OUTPUT_STROKE_WIDTH__1(_builtin_OUTPUT_STROKE_WIDTH__1)
    , OUTPUT_COLOR_3(_builtin_OUTPUT_COLOR_3)
    , OUTPUT_COLOR(_builtin_OUTPUT_COLOR)
    , OUTPUT_COLOR_2(_builtin_OUTPUT_COLOR_2)
    , OUTPUT_COLOR_0(_builtin_OUTPUT_COLOR_0)
    , OUTPUT_COLOR__1(_builtin_OUTPUT_COLOR__1)
    , OUTPUT_TEXT_BOLD(_builtin_OUTPUT_TEXT_BOLD)
    , OUTPUT_TEXT_ORDER(_builtin_OUTPUT_TEXT_ORDER)
    , OUTPUT_TEXT_MIN_DISTANCE(_builtin_OUTPUT_TEXT_MIN_DISTANCE)
    , OUTPUT_TEXT_ON_PATH(_builtin_OUTPUT_TEXT_ON_PATH)
    , OUTPUT_ICON(_builtin_OUTPUT_ICON)
    , OUTPUT_ORDER(_builtin_OUTPUT_ORDER)
    , OUTPUT_SHADOW_LEVEL(_builtin_OUTPUT_SHADOW_LEVEL)
    , OUTPUT_TEXT_DY(_builtin_OUTPUT_TEXT_DY)
    , OUTPUT_TEXT_SIZE(_builtin_OUTPUT_TEXT_SIZE)
    , OUTPUT_TEXT_COLOR(_builtin_OUTPUT_TEXT_COLOR)
    , OUTPUT_TEXT_HALO_RADIUS(_builtin_OUTPUT_TEXT_HALO_RADIUS)
    , OUTPUT_TEXT_WRAP_WIDTH(_builtin_OUTPUT_TEXT_WRAP_WIDTH)
    , OUTPUT_OBJECT_TYPE(_builtin_OUTPUT_OBJECT_TYPE)
    , OUTPUT_ATTR_INT_VALUE(_builtin_OUTPUT_ATTR_INT_VALUE)
    , OUTPUT_ATTR_COLOR_VALUE(_builtin_OUTPUT_ATTR_COLOR_VALUE)
    , OUTPUT_ATTR_BOOL_VALUE(_builtin_OUTPUT_ATTR_BOOL_VALUE)
    , OUTPUT_ATTR_STRING_VALUE(_builtin_OUTPUT_ATTR_STRING_VALUE)
{
    _name = QFileInfo(externalStyleFile.fileName()).fileName().replace(".render.xml", "");
    inflateBuiltinValues();
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

std::shared_ptr<OsmAnd::RasterizationStyle::ValueDefinition> OsmAnd::RasterizationStyle::registerBuiltinValue( ValueDefinition* pValueDefinition )
{
    assert(_valuesDefinitions.size() == 0 || _valuesDefinitions.size() <= _firstNonBuiltinValueDefinitionIndex);

    std::shared_ptr<ValueDefinition> valueDefinition(pValueDefinition);
    _valuesDefinitions.insert(pValueDefinition->name, valueDefinition);
    _firstNonBuiltinValueDefinitionIndex = _valuesDefinitions.size();
    return valueDefinition;
}

std::shared_ptr<OsmAnd::RasterizationStyle::ValueDefinition> OsmAnd::RasterizationStyle::registerValue( ValueDefinition* pValueDefinition )
{
    std::shared_ptr<ValueDefinition> valueDefinition(pValueDefinition);
    _valuesDefinitions.insert(pValueDefinition->name, valueDefinition);
    return valueDefinition;
}

void OsmAnd::RasterizationStyle::inflateBuiltinValues()
{
    _builtin_INPUT_TEST = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Boolean, "test"));
    _builtin_INPUT_TAG = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::String, "tag"));
    _builtin_INPUT_VALUE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::String, "value"));
    _builtin_INPUT_ADDITIONAL = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::String, "additional"));
    _builtin_INPUT_MINZOOM = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Integer, "minzoom"));
    _builtin_INPUT_MAXZOOM = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Integer, "maxzoom"));
    _builtin_INPUT_NIGHT_MODE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Boolean, "nightMode"));
    _builtin_INPUT_LAYER = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Integer, "layer"));
    _builtin_INPUT_POINT = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Boolean, "point"));
    _builtin_INPUT_AREA = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Boolean, "area"));
    _builtin_INPUT_CYCLE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Boolean, "cycle"));

    _builtin_INPUT_TEXT_LENGTH = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::Integer, "textLength"));
    _builtin_INPUT_NAME_TAG = registerBuiltinValue(new ValueDefinition(ValueDefinition::Input, ValueDefinition::String, "nameTag"));

    _builtin_OUTPUT_ATTR_INT_VALUE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "attrIntValue"));
    _builtin_OUTPUT_ATTR_BOOL_VALUE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Boolean, "attrBoolValue"));
    _builtin_OUTPUT_ATTR_COLOR_VALUE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "attrColorValue"));
    _builtin_OUTPUT_ATTR_STRING_VALUE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "attrStringValue"));

    // order - no sense to make it float
    _builtin_OUTPUT_ORDER = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "order"));
    _builtin_OUTPUT_OBJECT_TYPE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "objectType"));
    _builtin_OUTPUT_SHADOW_LEVEL = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "shadowLevel"));

    // text properties
    _builtin_OUTPUT_TEXT_WRAP_WIDTH = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "textWrapWidth"));
    _builtin_OUTPUT_TEXT_DY = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "textDy"));
    _builtin_OUTPUT_TEXT_HALO_RADIUS = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "textHaloRadius"));
    _builtin_OUTPUT_TEXT_SIZE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "textSize"));
    _builtin_OUTPUT_TEXT_ORDER = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "textOrder"));
    _builtin_OUTPUT_TEXT_MIN_DISTANCE = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "textMinDistance"));
    _builtin_OUTPUT_TEXT_SHIELD = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "textShield"));

    _builtin_OUTPUT_TEXT_COLOR = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "textColor"));
    _builtin_OUTPUT_TEXT_BOLD = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Boolean, "textBold"));
    _builtin_OUTPUT_TEXT_ON_PATH = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Boolean, "textOnPath"));

    // point
    _builtin_OUTPUT_ICON = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "icon"));

    // polygon/way
    _builtin_OUTPUT_COLOR = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "color"));
    _builtin_OUTPUT_COLOR_2 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "color_2"));
    _builtin_OUTPUT_COLOR_3 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "color_3"));
    _builtin_OUTPUT_COLOR_0 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "color_0"));
    _builtin_OUTPUT_COLOR__1 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "color__1"));
    _builtin_OUTPUT_STROKE_WIDTH = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Float, "strokeWidth"));
    _builtin_OUTPUT_STROKE_WIDTH_2 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Float, "strokeWidth_2"));
    _builtin_OUTPUT_STROKE_WIDTH_3 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Float, "strokeWidth_3"));
    _builtin_OUTPUT_STROKE_WIDTH_0 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Float, "strokeWidth_0"));
    _builtin_OUTPUT_STROKE_WIDTH__1 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Float, "strokeWidth__1"));

    _builtin_OUTPUT_PATH_EFFECT = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "pathEffect"));
    _builtin_OUTPUT_PATH_EFFECT_2 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "pathEffect_2"));
    _builtin_OUTPUT_PATH_EFFECT_3 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "pathEffect_3"));
    _builtin_OUTPUT_PATH_EFFECT_0 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "pathEffect_0"));
    _builtin_OUTPUT_PATH_EFFECT__1 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "pathEffect__1"));
    _builtin_OUTPUT_CAP = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "cap"));
    _builtin_OUTPUT_CAP_2 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "cap_2"));
    _builtin_OUTPUT_CAP_3 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "cap_3"));
    _builtin_OUTPUT_CAP_0 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "cap_0"));
    _builtin_OUTPUT_CAP__1 = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "cap__1"));

    _builtin_OUTPUT_SHADER = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::String, "shader"));
    _builtin_OUTPUT_SHADOW_COLOR = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Color, "shadowColor"));
    _builtin_OUTPUT_SHADOW_RADIUS = registerBuiltinValue(new ValueDefinition(ValueDefinition::Output, ValueDefinition::Integer, "shadowRadius"));
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

    auto tag = rule->getIntegerAttribute(INPUT_TAG->name, &ok);
    if(!ok)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
        return false;
    }

    auto value = rule->getIntegerAttribute(INPUT_VALUE->name, &ok);
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
