#include "ResolvedMapStyle_P.h"
#include "ResolvedMapStyle.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"
#include "QtCommon.h"

#include "MapStyleValueDefinition.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::ResolvedMapStyle_P::ResolvedMapStyle_P(ResolvedMapStyle* const owner_)
    : owner(owner_)
{
}

OsmAnd::ResolvedMapStyle_P::~ResolvedMapStyle_P()
{
}

OsmAnd::ResolvedMapStyle_P::StringId OsmAnd::ResolvedMapStyle_P::addStringToLUT(const QString& value)
{
    const auto newId = static_cast<OsmAnd::ResolvedMapStyle_P::StringId>(_stringsForwardLUT.size());

    _stringsBackwardLUT.insert(value, newId);
    _stringsForwardLUT.push_back(value);

    return newId;
}

OsmAnd::ResolvedMapStyle_P::StringId OsmAnd::ResolvedMapStyle_P::resolveStringIdInLUT(const QString& value)
{
    const auto citStringId = _stringsBackwardLUT.constFind(value);
    if (citStringId == _stringsBackwardLUT.cend())
        return addStringToLUT(value);
    return *citStringId;
}

bool OsmAnd::ResolvedMapStyle_P::resolveStringIdInLUT(const QString& value, StringId& outId) const
{
    const auto citStringId = _stringsBackwardLUT.constFind(value);
    if (citStringId == _stringsBackwardLUT.cend())
        return false;

    outId = *citStringId;
    return true;
}

bool OsmAnd::ResolvedMapStyle_P::parseConstantValue(
    const QString& input,
    const MapStyleValueDataType dataType,
    const bool isComplex,
    MapStyleConstantValue& outValue) const
{
    if (input.startsWith(QLatin1Char('$')))
    {
        const auto constantName = input.mid(1);
        QString constantValue;
        if (!resolveConstant(constantName, constantValue))
        {
            LogPrintf(LogSeverityLevel::Error,
                "Constant '%s' not defined",
                qPrintable(constantName));
            return false;
        }

        return parseConstantValue(constantValue, dataType, isComplex, outValue);
    }

    if (dataType == MapStyleValueDataType::String)
        return resolveStringIdInLUT(input, outValue.asSimple.asUInt);

    return MapStyleConstantValue::parse(input, dataType, isComplex, outValue);
}

bool OsmAnd::ResolvedMapStyle_P::parseConstantValue(
    const QString& input,
    const MapStyleValueDataType dataType,
    const bool isComplex,
    MapStyleConstantValue& outValue)
{
    if (input.startsWith(QLatin1Char('$')))
    {
        const auto constantName = input.mid(1);
        QString constantValue;
        if (!resolveConstant(constantName, constantValue))
        {
            LogPrintf(LogSeverityLevel::Error,
                "Constant '%s' not defined",
                qPrintable(constantName));
            return false;
        }

        return parseConstantValue(constantValue, dataType, isComplex, outValue);
    }

    if (dataType == MapStyleValueDataType::String)
    {
        outValue.asSimple.asUInt = resolveStringIdInLUT(input);
        return true;
    }

    return MapStyleConstantValue::parse(input, dataType, isComplex, outValue);
}

bool OsmAnd::ResolvedMapStyle_P::resolveValue(
    const QString& input,
    const MapStyleValueDataType dataType,
    const bool isComplex,
    ResolvedValue& outValue)
{
    if (input.startsWith(QLatin1Char('$')))
    {
        const auto constantOrAttributeName = input.mid(1);

        // Try to resolve as constant
        QString constantValue;
        if (resolveConstant(constantOrAttributeName, constantValue))
        {
            if (parseConstantValue(constantValue, dataType, isComplex, outValue.asConstantValue))
                return true;
        }

        // Try as attribute
        StringId attributeNameId = 0u;
        if (resolveStringIdInLUT(constantOrAttributeName, attributeNameId))
        {
            const auto citAttribute = _attributes.constFind(attributeNameId);
            if (citAttribute != _attributes.cend())
            {
                outValue.isDynamic = true;
                outValue.asDynamicValue.attribute = *citAttribute;
                return true;
            }
        }

        LogPrintf(LogSeverityLevel::Error,
            "'%s' was not defined nor as constant nor as attribute",
            qPrintable(constantOrAttributeName));
        return false;
    }

    if (parseConstantValue(input, dataType, isComplex, outValue.asConstantValue))
    {
        outValue.isDynamic = false;
        return true;
    }

    return false;
}

bool OsmAnd::ResolvedMapStyle_P::resolveConstant(const QString& name, QString& value) const
{
    const auto citConstantValue = _constants.constFind(name);
    if (citConstantValue == _constants.cend())
        return false;

    value = *citConstantValue;
    return true;
}

void OsmAnd::ResolvedMapStyle_P::registerBuiltinValueDefinitions()
{
    const auto builtinValueDefs = MapStyleBuiltinValueDefinitions::get();

    // Index of built-in value definition in _valuesDefinitions list always match it's id
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex)                             \
        _valuesDefinitions.push_back(builtinValueDefs->varname);                                                \
        _valuesDefinitionsIndicesByName.insert(QLatin1String(name), builtinValueDefs->varname->id);             \
        assert(_valuesDefinitions.size() - 1 == builtinValueDefs->varname->id);
#   include "MapStyleBuiltinValueDefinitions_Set.h"
#   undef DECLARE_BUILTIN_VALUEDEF
}

bool OsmAnd::ResolvedMapStyle_P::collectConstants()
{
    // Process styles chain in reverse order, top-most parent is the last element
    auto citUnresolvedMapStyle = iteratorOf(owner->unresolvedMapStylesChain);
    citUnresolvedMapStyle.toBack();
    while (citUnresolvedMapStyle.hasPrevious())
    {
        const auto& unresolvedMapStyle = citUnresolvedMapStyle.previous();

        for (const auto& constantEntry : rangeOf(constOf(unresolvedMapStyle->constants)))
            _constants[constantEntry.key()] = constantEntry.value();
    }

    return true;
}

std::shared_ptr<OsmAnd::ResolvedMapStyle_P::RuleNode> OsmAnd::ResolvedMapStyle_P::resolveRuleNode(
    const std::shared_ptr<const UnresolvedMapStyle::RuleNode>& unresolvedRuleNode)
{
    const std::shared_ptr<RuleNode> resolvedRuleNode(new RuleNode(
        unresolvedRuleNode->isSwitch));

    // Resolve values
    for (const auto& itUnresolvedValueEntry : rangeOf(constOf(unresolvedRuleNode->values)))
    {
        const auto& name = itUnresolvedValueEntry.key();
        const auto& value = itUnresolvedValueEntry.value();

        // Find value definition id and object
        const auto valueDefId = getValueDefinitionIdByName(name);
        const auto valueDef = getValueDefinitionById(valueDefId);
        if (valueDefId < 0 || !valueDef)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Ignoring unknown value '%s' = '%s'",
                qPrintable(name),
                qPrintable(value));
            continue;
        }

        // Try to resolve value
        ResolvedValue resolvedValue;
        if (!resolveValue(value, valueDef->dataType, valueDef->isComplex, resolvedValue))
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Ignoring value for '%s' since '%s' can not be resolved",
                qPrintable(name),
                qPrintable(value));
            continue;
        }

        resolvedRuleNode->values[valueDefId] = resolvedValue;
    }

    // <switch>/<case> subnodes
    for (const auto& unresolvedChild : constOf(unresolvedRuleNode->oneOfConditionalSubnodes))
    {
        const auto resolvedChild = resolveRuleNode(unresolvedChild);
        if (!resolvedChild)
            return nullptr;

        resolvedRuleNode->oneOfConditionalSubnodes.push_back(resolvedChild);
    }

    // <apply> subnodes
    for (const auto& unresolvedChild : constOf(unresolvedRuleNode->applySubnodes))
    {
        const auto resolvedChild = resolveRuleNode(unresolvedChild);
        if (!resolvedChild)
            return nullptr;

        resolvedRuleNode->applySubnodes.push_back(resolvedChild);
    }

    return resolvedRuleNode;
}

bool OsmAnd::ResolvedMapStyle_P::mergeAndResolveParameters()
{
    // Process styles chain in direct order, to exclude redefined parameters
    auto citUnresolvedMapStyle = iteratorOf(owner->unresolvedMapStylesChain);
    while (citUnresolvedMapStyle.hasNext())
    {
        const auto& unresolvedMapStyle = citUnresolvedMapStyle.next();

        for (const auto& unresolvedParameter : constOf(unresolvedMapStyle->parameters))
        {
            const auto nameId = resolveStringIdInLUT(unresolvedParameter->name);
            auto& resolvedParameter = _parameters[nameId];

            // Skip already defined parameters
            if (resolvedParameter)
                continue;

            // Resolve possible values
            QList<MapStyleConstantValue> resolvedPossibleValues;
            for (const auto& possibleValue : constOf(unresolvedParameter->possibleValues))
            {
                MapStyleConstantValue resolvedPossibleValue;
                if (!parseConstantValue(possibleValue, unresolvedParameter->dataType, false, resolvedPossibleValue))
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Failed to parse '%s' as possible value for '%s' (%s)",
                        qPrintable(possibleValue),
                        qPrintable(unresolvedParameter->name),
                        qPrintable(unresolvedParameter->title));
                    return false;
                }

                resolvedPossibleValues.push_back(qMove(resolvedPossibleValue));
            }

            // Create new resolved parameter
            resolvedParameter.reset(new Parameter(
                unresolvedParameter->title,
                unresolvedParameter->description,
                unresolvedParameter->category,
                nameId,
                unresolvedParameter->dataType,
                resolvedPossibleValues));

            // Register parameter as value definition
            const auto newValueDefId = _valuesDefinitions.size();
            const std::shared_ptr<ParameterValueDefinition> inputValueDefinition(new ParameterValueDefinition(
                newValueDefId,
                unresolvedParameter->name,
                resolvedParameter));
            _valuesDefinitions.push_back(inputValueDefinition);
            _valuesDefinitionsIndicesByName.insert(unresolvedParameter->name, newValueDefId);
        }
    }

    return true;
}

bool OsmAnd::ResolvedMapStyle_P::mergeAndResolveAttributes()
{
    QHash<StringId, std::shared_ptr<Attribute> > attributes;

    // Process styles chain in direct order to ensure "overridden" <case> elements are processed first
    auto citUnresolvedMapStyle = iteratorOf(owner->unresolvedMapStylesChain);
    while (citUnresolvedMapStyle.hasNext())
    {
        const auto& unresolvedMapStyle = citUnresolvedMapStyle.next();

        for (const auto& unresolvedAttribute : constOf(unresolvedMapStyle->attributes))
        {
            const auto nameId = resolveStringIdInLUT(unresolvedAttribute->name);
            auto& resolvedAttribute = attributes[nameId];

            // Create resolved attribute if needed
            if (!resolvedAttribute)
                resolvedAttribute.reset(new Attribute(nameId));

            const auto resolvedRuleNode = resolveRuleNode(unresolvedAttribute->rootNode);
            resolvedAttribute->rootNode->oneOfConditionalSubnodes.append(resolvedRuleNode->oneOfConditionalSubnodes);
        }
    }

    _attributes = copyAs< StringId, std::shared_ptr<const Attribute> >(attributes);

    return true;
}

bool OsmAnd::ResolvedMapStyle_P::mergeAndResolveRulesets()
{
    const auto builtinValueDefs = MapStyleBuiltinValueDefinitions::get();

    for (auto rulesetTypeIdx = 0u; rulesetTypeIdx < MapStyleRulesetTypesCount; rulesetTypeIdx++)
    {
        QHash<TagValueId, std::shared_ptr<Rule> > ruleset;

        // Process styles chain in direct order. This will allow to process overriding correctly
        auto citUnresolvedMapStyle = iteratorOf(owner->unresolvedMapStylesChain);
        while (citUnresolvedMapStyle.hasNext())
        {
            const auto& unresolvedMapStyle = citUnresolvedMapStyle.next();

            const auto& unresolvedRuleset = unresolvedMapStyle->rulesets[rulesetTypeIdx];
            auto itUnresolvedByTag = iteratorOf(unresolvedRuleset);
            while (itUnresolvedByTag.hasNext())
            {
                const auto& unresolvedByTagEntry = itUnresolvedByTag.next();
                const auto& tag = unresolvedByTagEntry.key();
                const auto& unresolvedByTag = unresolvedByTagEntry.value();

                const auto tagId = resolveStringIdInLUT(tag);

                auto itUnresolvedByValue = iteratorOf(unresolvedByTag);
                while (itUnresolvedByValue.hasNext())
                {
                    const auto& unresolvedByValueEntry = itUnresolvedByValue.next();
                    const auto& value = unresolvedByValueEntry.key();
                    const auto& unresolvedRule = unresolvedByValueEntry.value();

                    const auto valueId = resolveStringIdInLUT(value);

                    const auto tagValueId = TagValueId::compose(tagId, valueId);
                    auto& topLevelRule = ruleset[tagValueId];

                    // Create top-level rule if not yet created
                    if (!topLevelRule)
                    {
                        topLevelRule.reset(new Rule(static_cast<MapStyleRulesetType>(rulesetTypeIdx)));

                        topLevelRule->rootNode->values[builtinValueDefs->id_INPUT_TAG] =
                            ResolvedValue::fromConstantValue(MapStyleConstantValue::fromSimpleUInt(tagId));
                        topLevelRule->rootNode->values[builtinValueDefs->id_INPUT_VALUE] =
                            ResolvedValue::fromConstantValue(MapStyleConstantValue::fromSimpleUInt(valueId));
                    }

                    const auto resolvedRuleNode = resolveRuleNode(unresolvedRule->rootNode);
                    if (!resolvedRuleNode)
                    {
                        LogPrintf(LogSeverityLevel::Warning,
                            "One of rules for '%s':'%s' was ignored due to failure to resolve it",
                            qPrintable(tag),
                            qPrintable(value));
                        return false;
                    }
                    topLevelRule->rootNode->oneOfConditionalSubnodes.append(resolvedRuleNode->oneOfConditionalSubnodes);
                }
            }

            _rulesets[rulesetTypeIdx] = copyAs< TagValueId, std::shared_ptr<const Rule> >(ruleset);
        }
    }

    return true;
}

bool OsmAnd::ResolvedMapStyle_P::resolve()
{
    // Empty string always have 0 identifier
    addStringToLUT(QString());

    if (!collectConstants())
        return false;

    // Register built-in value definitions first
    registerBuiltinValueDefinitions();
    if (!mergeAndResolveParameters())
        return false;

    if (!mergeAndResolveAttributes())
        return false;

    if (!mergeAndResolveRulesets())
        return false;

    return true;
}

OsmAnd::ResolvedMapStyle_P::ValueDefinitionId OsmAnd::ResolvedMapStyle_P::getValueDefinitionIdByName(const QString& name) const
{
    const auto citId = _valuesDefinitionsIndicesByName.constFind(name);
    if (citId == _valuesDefinitionsIndicesByName.cend())
        return -1;
    return *citId;
}

std::shared_ptr<const OsmAnd::MapStyleValueDefinition> OsmAnd::ResolvedMapStyle_P::getValueDefinitionById(
    const ValueDefinitionId id) const
{
    if (id < 0 || id >= _valuesDefinitions.size())
        return nullptr;
    return _valuesDefinitions[id];
}

QList< std::shared_ptr<const OsmAnd::MapStyleValueDefinition> > OsmAnd::ResolvedMapStyle_P::getValueDefinitions() const
{
    return detachedOf(_valuesDefinitions);
}

bool OsmAnd::ResolvedMapStyle_P::parseConstantValue(
    const QString& input,
    const ValueDefinitionId valueDefintionId,
    MapStyleConstantValue& outParsedValue) const
{
    if (valueDefintionId < 0 || valueDefintionId >= _valuesDefinitions.size())
        return false;
    return parseConstantValue(input, _valuesDefinitions[valueDefintionId], outParsedValue);
}

bool OsmAnd::ResolvedMapStyle_P::parseConstantValue(
    const QString& input,
    const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion,
    MapStyleConstantValue& outParsedValue) const
{
    return parseConstantValue(input, valueDefintion->dataType, valueDefintion->isComplex, outParsedValue);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle_P::Attribute> OsmAnd::ResolvedMapStyle_P::getAttribute(const QString& name) const
{
    StringId nameId;
    if (!resolveStringIdInLUT(name, nameId))
        return nullptr;

    return _attributes[nameId];
}

const QHash< OsmAnd::TagValueId, std::shared_ptr<const OsmAnd::ResolvedMapStyle_P::Rule> >
OsmAnd::ResolvedMapStyle_P::getRuleset(
const MapStyleRulesetType rulesetType) const
{
    return _rulesets[static_cast<unsigned int>(rulesetType)];
}

QString OsmAnd::ResolvedMapStyle_P::getStringById(const StringId id) const
{
    if (id >= _stringsForwardLUT.size())
        return QString::null;
    return _stringsForwardLUT[id];
}

QString OsmAnd::ResolvedMapStyle_P::dump(const QString& prefix) const
{
    QString dump;

    const auto decodeMapValueDataType =
        []
        (const MapStyleValueDataType dataType) -> QString
        {
            switch (dataType)
            {
                case MapStyleValueDataType::Boolean:
                    return QLatin1String("boolean");
                case MapStyleValueDataType::Integer:
                    return QLatin1String("integer");
                case MapStyleValueDataType::Float:
                    return QLatin1String("float");
                case MapStyleValueDataType::String:
                    return QLatin1String("string");
                case MapStyleValueDataType::Color:
                    return QLatin1String("color");
            }

            return QLatin1String("unknown");
        };

    // Constants
    dump += prefix + QLatin1String("// Constants:\n");
    for (const auto& constant : rangeOf(constOf(_constants)))
        dump += prefix + QString(QLatin1String("def %1 = \"%2\";\n")).arg(constant.key()).arg(constant.value());
    dump += prefix + QLatin1String("\n");

    // Parameters
    dump += prefix + QLatin1String("// Parameters:\n");
    for (const auto& parameter : constOf(_parameters))
    {
        if (parameter->possibleValues.isEmpty())
        {
            dump += prefix + QString(QLatin1String("param %1 %2; // %3 (%4) \n"))
                .arg(decodeMapValueDataType(parameter->dataType))
                .arg(getStringById(parameter->nameId))
                .arg(parameter->title)
                .arg(parameter->description);
        }
        else
        {
            QStringList decodedPossibleValues;
            for (const auto possibleValue : constOf(parameter->possibleValues))
                decodedPossibleValues.append(dumpConstantValue(possibleValue, parameter->dataType));

            dump += prefix + QString(QLatin1String("param %1 %2 = (%3); // %4 (%5) \n"))
                .arg(decodeMapValueDataType(parameter->dataType))
                .arg(getStringById(parameter->nameId))
                .arg(decodedPossibleValues.join(QLatin1String(" | ")))
                .arg(parameter->title)
                .arg(parameter->description);
        }
    }
    dump += prefix + QLatin1String("\n");

    // Attributes
    dump += prefix + QLatin1String("// Attributes:\n");
    for (const auto& attribute : constOf(_attributes))
    {
        dump += prefix + QString(QLatin1String("attribute %1:\n")).arg(getStringById(attribute->nameId));
        dump += dumpRuleNode(attribute->rootNode, false, prefix + QLatin1String("\t"));
    }
    dump += prefix + QLatin1String("\n");

    // Order rules
    dump += prefix + QLatin1String("// Order rules:\n");
    for (const auto& ruleEntry : rangeOf(constOf(_rulesets[static_cast<unsigned int>(MapStyleRulesetType::Order)])))
    {
        dump += prefix + QString(QLatin1String("order %1 = %2:\n"))
            .arg(getStringById(ruleEntry.key().tagId))
            .arg(getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(ruleEntry.value()->rootNode, true, prefix + QLatin1String("\t"));
    }
    dump += prefix + QLatin1String("\n");

    // Point rules
    dump += prefix + QLatin1String("// Point rules:\n");
    for (const auto& ruleEntry : rangeOf(constOf(_rulesets[static_cast<unsigned int>(MapStyleRulesetType::Point)])))
    {
        dump += prefix + QString(QLatin1String("point %1 = %2:\n"))
            .arg(getStringById(ruleEntry.key().tagId))
            .arg(getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(ruleEntry.value()->rootNode, true, prefix + QLatin1String("\t"));
    }
    dump += prefix + QLatin1String("\n");

    // Polyline rules
    dump += prefix + QLatin1String("// Polyline rules:\n");
    for (const auto& ruleEntry : rangeOf(constOf(_rulesets[static_cast<unsigned int>(MapStyleRulesetType::Polyline)])))
    {
        dump += prefix + QString(QLatin1String("polyline %1 = %2:\n"))
            .arg(getStringById(ruleEntry.key().tagId))
            .arg(getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(ruleEntry.value()->rootNode, true, prefix + QLatin1String("\t"));
    }
    dump += prefix + QLatin1String("\n");

    // Polygon rules
    dump += prefix + QLatin1String("// Polygon rules:\n");
    for (const auto& ruleEntry : rangeOf(constOf(_rulesets[static_cast<unsigned int>(MapStyleRulesetType::Polygon)])))
    {
        dump += prefix + QString(QLatin1String("polygon %1 = %2:\n"))
            .arg(getStringById(ruleEntry.key().tagId))
            .arg(getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(ruleEntry.value()->rootNode, true, prefix + QLatin1String("\t"));
    }
    dump += prefix + QLatin1String("\n");

    // Text rules
    dump += prefix + QLatin1String("// Text rules:\n");
    for (const auto& ruleEntry : rangeOf(constOf(_rulesets[static_cast<unsigned int>(MapStyleRulesetType::Text)])))
    {
        dump += prefix + QString(QLatin1String("text %1 = %2:\n"))
            .arg(getStringById(ruleEntry.key().tagId))
            .arg(getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(ruleEntry.value()->rootNode, true, prefix + QLatin1String("\t"));
    }
    dump += prefix + QLatin1String("\n");

    return dump;
}

QString OsmAnd::ResolvedMapStyle_P::dumpRuleNode(
    const std::shared_ptr<const RuleNode>& ruleNode,
    const bool rejectSupported,
    const QString& prefix) const
{
    QString dump;
    const auto builtinValueDefs = MapStyleBuiltinValueDefinitions::get();

    bool hasInputValueTests = false;
    bool hasDisable = false;
    for (const auto& ruleValueEntry : rangeOf(constOf(ruleNode->values)))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = getValueDefinitionById(valueDefId);

        if (valueDef->valueClass == MapStyleValueDefinition::Class::Input)
            hasInputValueTests = true;
        else if (valueDef->valueClass == MapStyleValueDefinition::Class::Output && valueDef->name == QLatin1String("disable"))
            hasDisable = true;

        if (hasInputValueTests && hasDisable)
            break;
    }

    if (hasInputValueTests)
    {
        dump += prefix + QLatin1String("local testPassed = true;\n");
        for (const auto& ruleValueEntry : rangeOf(constOf(ruleNode->values)))
        {
            const auto valueDefId = ruleValueEntry.key();
            const auto& valueDef = getValueDefinitionById(valueDefId);

            // Test only input values
            if (valueDef->valueClass != MapStyleValueDefinition::Class::Input)
                continue;

            const auto& resolvedRuleValue = ruleValueEntry.value();

            //bool evaluationResult = false;
            if (valueDefId == builtinValueDefs->id_INPUT_MINZOOM)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 <= %2);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::Integer))
                    .arg(valueDef->name);
            }
            else if (valueDefId == builtinValueDefs->id_INPUT_MAXZOOM)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 >= %2);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::Integer))
                    .arg(valueDef->name);
            }
            else if (valueDefId == builtinValueDefs->id_INPUT_ADDITIONAL)
            {
                dump += prefix + QString(QLatin1String("if (testPassed and inputMapObject) testPassed = (inputMapObject contains %1);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::String));
            }
            else if (valueDefId == builtinValueDefs->id_INPUT_TEST)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == 1);\n"))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == MapStyleValueDataType::Float)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::Float))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == MapStyleValueDataType::Integer)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::Integer))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == MapStyleValueDataType::Boolean)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::Boolean))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == MapStyleValueDataType::String)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(resolvedRuleValue, MapStyleValueDataType::String))
                    .arg(valueDef->name);
            }
            else
            {
                dump += prefix + QString(QLatin1String("// Unknown condition with '%1'\n"))
                    .arg(valueDef->name);
            }
        }
        if (rejectSupported)
            dump += prefix + QLatin1String("if (not testPassed) reject;\n");
        else
            dump += prefix + QLatin1String("if (not testPassed) exit;\n");
        dump += prefix + QLatin1String("\n");
    }

    if (hasDisable)
    {
        if (rejectSupported)
            dump += prefix + QLatin1String("if (disable) reject;\n");
        else
            dump += prefix + QLatin1String("if (disable) exit;\n");
        dump += prefix + QLatin1String("\n");
    }

    if (!ruleNode->isSwitch)
        dump += dumpRuleNodeOutputValues(ruleNode, prefix, true);

    if (!ruleNode->oneOfConditionalSubnodes.isEmpty())
    {
        dump += prefix + QLatin1String("local atLeastOneConditionalMatched = false;\n");
        for (const auto& oneOfConditionalSubnode : constOf(ruleNode->oneOfConditionalSubnodes))
        {
            dump += prefix + QLatin1String("if (not atLeastOneConditionalMatched):\n");
            dump += dumpRuleNode(oneOfConditionalSubnode, false, prefix + QLatin1String("\t"));
            dump += prefix + QLatin1String("\tatLeastOneConditionalMatched = true;\n");
        }
        if (ruleNode->isSwitch)
        {
            if (rejectSupported)
                dump += prefix + QLatin1String("if (not atLeastOneConditionalMatched) reject;\n");
            else
                dump += prefix + QLatin1String("if (not atLeastOneConditionalMatched) exit;\n");
        }
        dump += prefix + QLatin1String("\n");
    }

    if (ruleNode->isSwitch)
        dump += dumpRuleNodeOutputValues(ruleNode, prefix, false);
    
    if (!ruleNode->applySubnodes.isEmpty())
    {
        for (const auto& applySubnode : constOf(ruleNode->applySubnodes))
        {
            dump += prefix + QLatin1String("apply:\n");
            dump += dumpRuleNode(applySubnode, false, prefix + QLatin1String("\t"));
        }
        dump += prefix + QLatin1String("\n");
    }

    return dump;
}

QString OsmAnd::ResolvedMapStyle_P::dumpRuleNodeOutputValues(
    const std::shared_ptr<const RuleNode>& ruleNode,
    const QString& prefix,
    const bool allowOverride) const
{
    QString dump;
    const auto builtinValueDefs = MapStyleBuiltinValueDefinitions::get();

    for (const auto& ruleValueEntry : rangeOf(constOf(ruleNode->values)))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = getValueDefinitionById(valueDefId);

        // Skip all non-Output values
        if (valueDef->valueClass != MapStyleValueDefinition::Class::Output)
            continue;

        const auto& resolvedRuleValue = ruleValueEntry.value();

        dump += prefix + QString(QLatin1String("%1 %2 = %3;\n"))
            .arg(allowOverride ? QLatin1String("setOrOverride") : QLatin1String("setIfNotSet"))
            .arg(valueDef->name)
            .arg(dumpResolvedValue(resolvedRuleValue, valueDef->dataType));
    }

    return dump;
}

QString OsmAnd::ResolvedMapStyle_P::dumpResolvedValue(const ResolvedValue& value, const MapStyleValueDataType dataType) const
{
    if (value.isDynamic)
    {
        const auto attributeName = QString(QLatin1String("attribute $%1"))
            .arg(getStringById(value.asDynamicValue.attribute->nameId));

        switch (dataType)
        {
            case MapStyleValueDataType::Boolean:
                return QString(QLatin1String("boolean(%1)")).arg(attributeName);

            case MapStyleValueDataType::Integer:
                return QString(QLatin1String("integer(%1)")).arg(attributeName);

            case MapStyleValueDataType::Float:
                return QString(QLatin1String("float(%1)")).arg(attributeName);

            case MapStyleValueDataType::String:
                return QString(QLatin1String("string(%1)")).arg(attributeName);

            case MapStyleValueDataType::Color:
                return QString(QLatin1String("color(%1)")).arg(attributeName);
        }

        return QString::null;
    }

    return dumpConstantValue(value.asConstantValue, dataType);
}

QString OsmAnd::ResolvedMapStyle_P::dumpConstantValue(
    const MapStyleConstantValue& value,
    const MapStyleValueDataType dataType) const
{
    if (dataType == MapStyleValueDataType::String)
        return QString(QLatin1String("string(\"%1\")")).arg(getStringById(value.asSimple.asInt));

    return value.toString(dataType);
}
