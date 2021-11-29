#include "CStyle.h"

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iomanip>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/QtCommon.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QTextStream>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>

#include <OsmAndCoreTools.h>
#include <OsmAndCoreTools/Utilities.h>

OsmAndTools::CStyle::CStyle(const Configuration& configuration_)
    : configuration(configuration_)
{
}

OsmAndTools::CStyle::~CStyle()
{
}

#if defined(_UNICODE) || defined(UNICODE)
bool OsmAndTools::CStyle::compile(std::wostream& output)
#else
bool OsmAndTools::CStyle::compile(std::ostream& output)
#endif
{
    bool success = true;
    for (;;)
    {
        // Find style
        const auto mapStyle = configuration.stylesCollection->getResolvedStyleByName(configuration.styleName);
        if (!mapStyle)
        {
            output
                << "Failed to resolve style '"
                << QStringToStlString(configuration.styleName)
                << "' from collection:"
                << std::endl;
            for (const auto& style : configuration.stylesCollection->getCollection())
            {
                if (style->isMetadataLoaded())
                {
                    if (style->isStandalone())
                        output << "\t" << QStringToStlString(style->name) << std::endl;
                    else
                    {
                        output
                            << "\t"
                            << QStringToStlString(style->name)
                            << "::"
                            << QStringToStlString(style->parentName)
                            << std::endl;
                    }
                }
                else
                    output << "\t[missing metadata]" << std::endl;
            }

            success = false;
            break;
        }

        std::shared_ptr<BaseEmitter> emitter = nullptr;
        switch (configuration.emitterDialect)
        {
            case EmitterDialect::Debug:
                emitter.reset(new DebugEmitter(output));
                break;
        }
        if (!emitter)
        {
            success = false;
            break;
        }
        success = emitter->emitStyle(mapStyle);

        break;
    }

    return success;
}

bool OsmAndTools::CStyle::compile(QString *pLog /*= nullptr*/)
{
    if (pLog != nullptr)
    {
#if defined(_UNICODE) || defined(UNICODE)
        std::wostringstream output;
        const bool success = compile(output);
        *pLog = QString::fromStdWString(output.str());
        return success;
#else
        std::ostringstream output;
        const bool success = compile(output);
        *pLog = QString::fromStdString(output.str());
        return success;
#endif
    }
    else
    {
#if defined(_UNICODE) || defined(UNICODE)
        return compile(std::wcout);
#else
        return compile(std::cout);
#endif
    }
}

OsmAndTools::CStyle::Configuration::Configuration()
    : styleName(QLatin1String("default"))
    , emitterDialect(EmitterDialect::Debug)
    , verbose(false)
{
}

bool OsmAndTools::CStyle::Configuration::parseFromCommandLineArguments(
    const QStringList& commandLineArgs,
    Configuration& outConfiguration,
    QString& outError)
{
    outConfiguration = Configuration();

    const std::shared_ptr<OsmAnd::MapStylesCollection> stylesCollection(new OsmAnd::MapStylesCollection());
    outConfiguration.stylesCollection = stylesCollection;

    for (const auto& arg : commandLineArgs)
    {
        if (arg.startsWith(QLatin1String("-stylesPath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-stylesPath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            QFileInfoList styleFilesList;
            OsmAnd::Utilities::findFiles(QDir(value), QStringList() << QLatin1String("*.render.xml"), styleFilesList, false);
            for (const auto& styleFile : styleFilesList)
                stylesCollection->addStyleFromFile(styleFile.absoluteFilePath());
        }
        else if (arg.startsWith(QLatin1String("-stylesRecursivePath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-stylesRecursivePath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            QFileInfoList styleFilesList;
            OsmAnd::Utilities::findFiles(QDir(value), QStringList() << QLatin1String("*.render.xml"), styleFilesList, true);
            for (const auto& styleFile : styleFilesList)
                stylesCollection->addStyleFromFile(styleFile.absoluteFilePath());
        }
        else if (arg.startsWith(QLatin1String("-styleName=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-styleName=")));
            outConfiguration.styleName = value;
        }
        else if (arg.startsWith(QLatin1String("-emitter=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-emitter=")));
            if (value == QLatin1String("debug"))
                outConfiguration.emitterDialect = EmitterDialect::Debug;
            else
            {
                outError = QString("'%1' emitter is not supported").arg(value);
                return false;
            }
        }
        else if (arg == QLatin1String("-verbose"))
        {
            outConfiguration.verbose = true;
        }
        else
        {
            outError = QString("Unrecognized argument: '%1'").arg(arg);
            return false;
        }
    }

    // Validate
    if (outConfiguration.styleName.isEmpty())
    {
        outError = QLatin1String("'styleName' can not be empty");
        return false;
    }

    return true;
}

#if defined(_UNICODE) || defined(UNICODE)
OsmAndTools::CStyle::BaseEmitter::BaseEmitter(std::wostream& output_)
#else
OsmAndTools::CStyle::BaseEmitter::BaseEmitter(std::ostream& output_)
#endif
    : output(output_)
{
}

OsmAndTools::CStyle::BaseEmitter::~BaseEmitter()
{
}

#if defined(_UNICODE) || defined(UNICODE)
OsmAndTools::CStyle::DebugEmitter::DebugEmitter(std::wostream& output_)
#else
OsmAndTools::CStyle::DebugEmitter::DebugEmitter(std::ostream& output_)
#endif
    : BaseEmitter(output_)
{
}

OsmAndTools::CStyle::DebugEmitter::~DebugEmitter()
{
}

bool OsmAndTools::CStyle::DebugEmitter::emitStyle(const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle)
{
    QString dump;

    const auto decodeMapValueDataType =
        []
        (const OsmAnd::MapStyleValueDataType dataType) -> QString
        {
            switch (dataType)
            {
                case OsmAnd::MapStyleValueDataType::Boolean:
                    return QLatin1String("boolean");
                case OsmAnd::MapStyleValueDataType::Integer:
                    return QLatin1String("integer");
                case OsmAnd::MapStyleValueDataType::Float:
                    return QLatin1String("float");
                case OsmAnd::MapStyleValueDataType::String:
                    return QLatin1String("string");
                case OsmAnd::MapStyleValueDataType::Color:
                    return QLatin1String("color");
            }

            return QLatin1String("unknown");
        };

    // Parameters
    dump += QLatin1String("// Parameters:\n\n");
    const auto& parameters = mapStyle->getParameters();
    for (const auto& parameter : parameters)
    {
        const auto& possibleValues = parameter->getPossibleValues();

        dump += QString(QLatin1String("// %1 (%2)\n"))
            .arg(parameter->getTitle())
            .arg(parameter->getDescription());
        if (possibleValues.isEmpty())
        {
            dump += QString(QLatin1String("param %1 %2;\n"))
                .arg(decodeMapValueDataType(parameter->getDataType()))
                .arg(mapStyle->getStringById(parameter->getNameId()));
        }
        else
        {
            QStringList decodedPossibleValues;
            for (const auto& possibleValue : constOf(possibleValues))
                decodedPossibleValues.append(dumpConstantValue(mapStyle, possibleValue, parameter->getDataType()));

            dump += QString(QLatin1String("param %1 %2 = (%3);\n"))
                .arg(decodeMapValueDataType(parameter->getDataType()))
                .arg(mapStyle->getStringById(parameter->getNameId()))
                .arg(decodedPossibleValues.join(QLatin1String(" | ")));
        }
        dump += QLatin1String("\n");
    }
    dump += QLatin1String("\n");

    // Attributes
    dump += QLatin1String("// Attributes:\n\n");
    const auto& attributes = mapStyle->getAttributes();
    for (const auto& attribute : attributes)
    {
        dump += QString(QLatin1String("attribute %1:\n")).arg(mapStyle->getStringById(attribute->getNameId()));
        dump += dumpRuleNode(mapStyle, attribute->getRootNode(), false, QLatin1String("\t"));
        dump += QLatin1String("\n");
    }
    dump += QLatin1String("\n");

    // Order rules
    dump += QLatin1String("// Order rules:\n");
    const auto& orderRules = mapStyle->getRuleset(OsmAnd::MapStyleRulesetType::Order);
    for (const auto& ruleEntry : rangeOf(orderRules))
    {
        dump += QString(QLatin1String("order %1 = %2:\n"))
            .arg(mapStyle->getStringById(ruleEntry.key().tagId))
            .arg(mapStyle->getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(mapStyle, ruleEntry.value()->getRootNode(), true, QLatin1String("\t"));
    }
    dump += QLatin1String("\n");

    // Point rules
    dump += QLatin1String("// Point rules:\n");
    const auto& pointRules = mapStyle->getRuleset(OsmAnd::MapStyleRulesetType::Point);
    for (const auto& ruleEntry : rangeOf(pointRules))
    {
        dump += QString(QLatin1String("point %1 = %2:\n"))
            .arg(mapStyle->getStringById(ruleEntry.key().tagId))
            .arg(mapStyle->getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(mapStyle, ruleEntry.value()->getRootNode(), true, QLatin1String("\t"));
    }
    dump += QLatin1String("\n");

    // Polyline rules
    dump += QLatin1String("// Polyline rules:\n");
    const auto& polylineRules = mapStyle->getRuleset(OsmAnd::MapStyleRulesetType::Polyline);
    for (const auto& ruleEntry : rangeOf(polylineRules))
    {
        dump += QString(QLatin1String("polyline %1 = %2:\n"))
            .arg(mapStyle->getStringById(ruleEntry.key().tagId))
            .arg(mapStyle->getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(mapStyle, ruleEntry.value()->getRootNode(), true, QLatin1String("\t"));
    }
    dump += QLatin1String("\n");

    // Polygon rules
    dump += QLatin1String("// Polygon rules:\n");
    const auto& polygonRules = mapStyle->getRuleset(OsmAnd::MapStyleRulesetType::Polygon);
    for (const auto& ruleEntry : rangeOf(polygonRules))
    {
        dump += QString(QLatin1String("polygon %1 = %2:\n"))
            .arg(mapStyle->getStringById(ruleEntry.key().tagId))
            .arg(mapStyle->getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(mapStyle, ruleEntry.value()->getRootNode(), true, QLatin1String("\t"));
    }
    dump += QLatin1String("\n");

    // Text rules
    dump += QLatin1String("// Text rules:\n");
    const auto& textRules = mapStyle->getRuleset(OsmAnd::MapStyleRulesetType::Text);
    for (const auto& ruleEntry : rangeOf(textRules))
    {
        dump += QString(QLatin1String("text %1 = %2:\n"))
            .arg(mapStyle->getStringById(ruleEntry.key().tagId))
            .arg(mapStyle->getStringById(ruleEntry.key().valueId));
        dump += dumpRuleNode(mapStyle, ruleEntry.value()->getRootNode(), true, QLatin1String("\t"));
    }
    dump += QLatin1String("\n");

    output << QStringToStlString(dump);

    return true;
}

QString OsmAndTools::CStyle::DebugEmitter::dumpConstantValue(
    const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
    const OsmAnd::MapStyleConstantValue& value,
    const OsmAnd::MapStyleValueDataType dataType)
{
    if (dataType == OsmAnd::MapStyleValueDataType::String)
        return QString(QLatin1String("string(\"%1\")")).arg(mapStyle->getStringById(value.asSimple.asInt));

    return value.toString(dataType);
}

QString OsmAndTools::CStyle::DebugEmitter::dumpRuleNode(
    const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
    const std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode>& ruleNode,
    const bool rejectSupported,
    const QString& prefix)
{
    QString dump;
    const auto builtinValueDefs = OsmAnd::MapStyleBuiltinValueDefinitions::get();

    bool hasInputValueTests = false;
    bool hasDisable = false;
    const auto& values = ruleNode->getValues();
    for (const auto& ruleValueEntry : OsmAnd::rangeOf(values))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = mapStyle->getValueDefinitionById(valueDefId);

        if (valueDef->valueClass == OsmAnd::MapStyleValueDefinition::Class::Input)
            hasInputValueTests = true;
        else if (valueDef->valueClass == OsmAnd::MapStyleValueDefinition::Class::Output && valueDef->name == QLatin1String("disable"))
            hasDisable = true;

        if (hasInputValueTests && hasDisable)
            break;
    }

    if (hasInputValueTests)
    {
        dump += prefix + QLatin1String("local testPassed = true;\n");
        for (const auto& ruleValueEntry : OsmAnd::rangeOf(values))
        {
            const auto valueDefId = ruleValueEntry.key();
            const auto& valueDef = mapStyle->getValueDefinitionById(valueDefId);

            // Test only input values
            if (valueDef->valueClass != OsmAnd::MapStyleValueDefinition::Class::Input)
                continue;

            const auto& resolvedRuleValue = ruleValueEntry.value();

            //bool evaluationResult = false;
            if (valueDefId == builtinValueDefs->id_INPUT_MINZOOM)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 <= %2);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::Integer))
                    .arg(valueDef->name);
            }
            else if (valueDefId == builtinValueDefs->id_INPUT_MAXZOOM)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 >= %2);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::Integer))
                    .arg(valueDef->name);
            }
            else if (valueDefId == builtinValueDefs->id_INPUT_ADDITIONAL)
            {
                dump += prefix + QString(QLatin1String("if (testPassed and inputMapObject) testPassed = (inputMapObject contains %1);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::String));
            }
            else if (valueDefId == builtinValueDefs->id_INPUT_TEST)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == 1);\n"))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == OsmAnd::MapStyleValueDataType::Float)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::Float))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == OsmAnd::MapStyleValueDataType::Integer)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::Integer))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == OsmAnd::MapStyleValueDataType::Boolean)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::Boolean))
                    .arg(valueDef->name);
            }
            else if (valueDef->dataType == OsmAnd::MapStyleValueDataType::String)
            {
                dump += prefix + QString(QLatin1String("if (testPassed) testPassed = (%1 == %2);\n"))
                    .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, OsmAnd::MapStyleValueDataType::String))
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

    if (!ruleNode->getIsSwitch())
        dump += dumpRuleNodeOutputValues(mapStyle, ruleNode, prefix, true);

    const auto& oneOfConditionalSubnodes = ruleNode->getOneOfConditionalSubnodesRef();
    if (!oneOfConditionalSubnodes.isEmpty())
    {
        dump += prefix + QLatin1String("local atLeastOneConditionalMatched = false;\n");
        for (const auto& oneOfConditionalSubnode : oneOfConditionalSubnodes)
        {
            dump += prefix + QLatin1String("if (not atLeastOneConditionalMatched):\n");
            dump += dumpRuleNode(mapStyle, oneOfConditionalSubnode, false, prefix + QLatin1String("\t"));
            dump += prefix + QLatin1String("\tatLeastOneConditionalMatched = true;\n");
        }
        if (ruleNode->getIsSwitch())
        {
            if (rejectSupported)
                dump += prefix + QLatin1String("if (not atLeastOneConditionalMatched) reject;\n");
            else
                dump += prefix + QLatin1String("if (not atLeastOneConditionalMatched) exit;\n");
        }
        dump += prefix + QLatin1String("\n");
    }

    if (ruleNode->getIsSwitch())
        dump += dumpRuleNodeOutputValues(mapStyle, ruleNode, prefix, false);

    const auto& applySubnodes = ruleNode->getApplySubnodesRef();
    if (!applySubnodes.isEmpty())
    {
        for (const auto& applySubnode : applySubnodes)
        {
            dump += prefix + QLatin1String("apply:\n");
            dump += dumpRuleNode(mapStyle, applySubnode, false, prefix + QLatin1String("\t"));
        }
        dump += prefix + QLatin1String("\n");
    }

    return dump;
}

QString OsmAndTools::CStyle::DebugEmitter::dumpResolvedValue(
    const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
    const OsmAnd::IMapStyle::Value& value,
    const OsmAnd::MapStyleValueDataType dataType)
{
    if (value.isDynamic)
    {
        const auto attributeName = QString(QLatin1String("attribute $%1"))
            .arg(mapStyle->getStringById(value.asDynamicValue.attribute->getNameId()));

        switch (dataType)
        {
            case OsmAnd::MapStyleValueDataType::Boolean:
                return QString(QLatin1String("boolean(%1)")).arg(attributeName);

            case OsmAnd::MapStyleValueDataType::Integer:
                return QString(QLatin1String("integer(%1)")).arg(attributeName);

            case OsmAnd::MapStyleValueDataType::Float:
                return QString(QLatin1String("float(%1)")).arg(attributeName);

            case OsmAnd::MapStyleValueDataType::String:
                return QString(QLatin1String("string(%1)")).arg(attributeName);

            case OsmAnd::MapStyleValueDataType::Color:
                return QString(QLatin1String("color(%1)")).arg(attributeName);
        }

        return {};
    }

    return dumpConstantValue(mapStyle, value.asConstantValue, dataType);
}

QString OsmAndTools::CStyle::DebugEmitter::dumpRuleNodeOutputValues(
    const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
    const std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode>& ruleNode,
    const QString& prefix,
    const bool allowOverride)
{
    QString dump;
    const auto builtinValueDefs = OsmAnd::MapStyleBuiltinValueDefinitions::get();

    const auto& values = ruleNode->getValues();
    for (const auto& ruleValueEntry : rangeOf(values))
    {
        const auto valueDefId = ruleValueEntry.key();
        const auto& valueDef = mapStyle->getValueDefinitionById(valueDefId);

        // Skip all non-Output values
        if (valueDef->valueClass != OsmAnd::MapStyleValueDefinition::Class::Output)
            continue;

        const auto& resolvedRuleValue = ruleValueEntry.value();

        dump += prefix + QString(QLatin1String("%1 %2 = %3;\n"))
            .arg(allowOverride ? QLatin1String("setOrOverride") : QLatin1String("setIfNotSet"))
            .arg(valueDef->name)
            .arg(dumpResolvedValue(mapStyle, resolvedRuleValue, valueDef->dataType));
    }

    return dump;
}
