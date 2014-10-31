#include "Styler.h"

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iomanip>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/Stopwatch.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/CoreResourcesEmbeddedBundle.h>
#include <OsmAndCore/ObfDataInterface.h>
#include <OsmAndCore/QKeyValueIterator.h>
#include <OsmAndCore/Data/Model/BinaryMapObject.h>
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/Primitiviser.h>
#include <OsmAndCore/Map/MapStyleEvaluationResult.h>

#include <OsmAndCoreTools.h>
#include <OsmAndCoreTools/Utilities.h>

OsmAndTools::Styler::Styler(const Configuration& configuration_)
    : configuration(configuration_)
{
}

OsmAndTools::Styler::~Styler()
{
}

#if defined(_UNICODE) || defined(UNICODE)
bool OsmAndTools::Styler::evaluate(EvaluatedMapObjects& outEvaluatedMapObjects, std::wostream& output)
#else
bool OsmAndTools::Styler::evaluate(EvaluatedMapObjects& outEvaluatedMapObjects, std::ostream& output)
#endif
{
    bool success = true;
    for (;;)
    {
        // Find style
        if (configuration.verbose)
            output << xT("Resolving style '") << QStringToStlString(configuration.styleName) << xT("'...") << std::endl;
        const auto mapStyle = configuration.stylesCollection->getResolvedStyleByName(configuration.styleName);
        if (!mapStyle)
        {
            if (configuration.verbose)
            {
                output << "Failed to resolve style '" << QStringToStlString(configuration.styleName) << "' from collection:" << std::endl;
                for (const auto& style : configuration.stylesCollection->getCollection())
                {
                    if (style->isMetadataLoaded())
                    {
                        if (style->isStandalone())
                            output << "\t" << QStringToStlString(style->name) << std::endl;
                        else
                            output << "\t" << QStringToStlString(style->name) << "::" << QStringToStlString(style->parentName) << std::endl;
                    }
                    else
                        output << "\t[missing metadata]" << std::endl;
                }
            }
            else
            {
                output << "Failed to resolve style '" << QStringToStlString(configuration.styleName) << "' from collection:" << std::endl;
            }

            success = false;
            break;
        }

        // Load all map objects
        QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> > mapObjects;
        const auto mapObjectsFilterById =
            [this]
            (const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section,
            const uint64_t mapObjectId,
            const OsmAnd::AreaI& bbox,
            const OsmAnd::ZoomLevel firstZoomLevel,
            const OsmAnd::ZoomLevel lastZoomLevel) -> bool
            {
                return configuration.mapObjectsIds.contains(mapObjectId);
            };
        if (configuration.verbose)
        {
            if (configuration.mapObjectsIds.isEmpty())
                output << xT("Going to load all available map objects...") << std::endl;
            else
                output << xT("Going to load ") << configuration.mapObjectsIds.size() << xT(" map objects...") << std::endl;
        }
        const auto obfDataInterface = configuration.obfsCollection->obtainDataInterface();
        success = obfDataInterface->loadMapObjects(
            &mapObjects,
            nullptr,
            configuration.zoom,
            nullptr,
            configuration.mapObjectsIds.isEmpty() ? OsmAnd::FilterMapObjectsByIdFunction() : mapObjectsFilterById,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (!success)
        {
            if (configuration.verbose)
                output << xT("Failed to load map objects!") << std::endl;
            break;
        }
        if (configuration.verbose)
            output << xT("Loaded ") << mapObjects.size() << xT(" map objects") << std::endl;

        // Prepare all resources for map style evaluation
        if (configuration.verbose)
        {
            output
                << xT("Initializing map presentation environment with display density ")
                << configuration.displayDensityFactor
                << xT(" and locale '")
                << QStringToStlString(configuration.locale)
                << xT("'...") << std::endl;
        }
        const std::shared_ptr<OsmAnd::MapPresentationEnvironment> mapPresentationEnvironment(new OsmAnd::MapPresentationEnvironment(
            mapStyle,
            configuration.displayDensityFactor,
            configuration.locale));

        if (configuration.verbose)
            output << xT("Applying extra style settings to map presentation environment...") << std::endl;
        mapPresentationEnvironment->setSettings(configuration.styleSettings);

        // Create primitiviser
        const std::shared_ptr<OsmAnd::Primitiviser> primitiviser(new OsmAnd::Primitiviser(mapPresentationEnvironment));
        if (configuration.verbose)
            output << xT("Going to primitivise map objects ") << (configuration.excludeCoastlines ? xT("excluding") : xT("including")) << xT(" coastlines...") << std::endl;
        const auto primitivisedArea = configuration.excludeCoastlines
            ? primitiviser->primitiviseWithoutCoastlines(
                configuration.zoom,
                mapObjects)
            : primitiviser->primitiviseWithCoastlines(
                OsmAnd::AreaI(0, 0, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()),
                OsmAnd::PointI(65536, 65536),
                configuration.zoom,
                OsmAnd::MapFoundationType::Mixed,
                mapObjects);
        if (configuration.verbose)
            output << xT("Primitivised ") << primitivisedArea->primitivesGroups.size() << xT(" groups from ") << mapObjects.size() << xT(" map objects") << std::endl;

        // Obtain evaluated values for each group and print it
        for (const auto& primitivisedGroup : OsmAnd::constOf(primitivisedArea->primitivesGroups))
        {
            // Skip objects that were not requested
            if (!configuration.mapObjectsIds.isEmpty() && configuration.mapObjectsIds.contains(primitivisedGroup->sourceObject->id))
                continue;

            outEvaluatedMapObjects[primitivisedGroup->sourceObject] = primitivisedGroup;

            output << QStringToStlString(QString(80, QLatin1Char('-'))) << std::endl;
            output << QStringToStlString(primitivisedGroup->sourceObject->id.toString()) << std::endl;

            if (!primitivisedGroup->points.isEmpty())
            {
                output << primitivisedGroup->points.size() << xT(" point(s):") << std::endl;

                unsigned int pointPrimitiveIndex = 0u;
                for (const auto& pointPrimitive : OsmAnd::constOf(primitivisedGroup->points))
                {
                    output << xT("\tPoint #") << pointPrimitiveIndex << std::endl;
                    QString ruleTag;
                    QString ruleValue;
                    const auto typeRuleResolved = primitivisedGroup->sourceObject->obtainTagValueByTypeRuleIndex(pointPrimitive->typeRuleIdIndex, ruleTag, ruleValue);
                    if (typeRuleResolved)
                        output << xT("\t\tTag/value: ") << QStringToStlString(ruleTag) << xT(" = ") << QStringToStlString(ruleValue) << std::endl;
                    else
                        output << xT("\t\tTag/value: ") << pointPrimitive->typeRuleIdIndex << xT(" (failed to resolve)") << std::endl;
                    output << xT("\t\tZ order: ") << pointPrimitive->zOrder << std::endl;
                    output << xT("\t\tArea*2: ") << pointPrimitive->doubledArea << std::endl;
                    for (const auto& evaluatedValueEntry : OsmAnd::rangeOf(OsmAnd::constOf(pointPrimitive->evaluationResult.values)))
                    {
                        const auto valueDefinitionId = evaluatedValueEntry.key();
                        const auto value = evaluatedValueEntry.value();

                        const auto valueDefinition = mapStyle->getValueDefinitionById(valueDefinitionId);

                        output << xT("\t\t") << QStringToStlString(valueDefinition->name) << xT(" = ") << QStringToStlString(value.toString()) << std::endl;
                    }

                    pointPrimitiveIndex++;
                }
            }

            if (!primitivisedGroup->polylines.isEmpty())
            {
                output << primitivisedGroup->polylines.size() << xT(" polyline(s):") << std::endl;

                unsigned int polylinePrimitiveIndex = 0u;
                for (const auto& polylinePrimitive : OsmAnd::constOf(primitivisedGroup->polylines))
                {
                    output << xT("\tPolyline #") << polylinePrimitiveIndex << std::endl;
                    QString ruleTag;
                    QString ruleValue;
                    const auto typeRuleResolved = primitivisedGroup->sourceObject->obtainTagValueByTypeRuleIndex(polylinePrimitive->typeRuleIdIndex, ruleTag, ruleValue);
                    if (typeRuleResolved)
                        output << xT("\t\tTag/value: ") << QStringToStlString(ruleTag) << xT(" = ") << QStringToStlString(ruleValue) << std::endl;
                    else
                        output << xT("\t\tTag/value: ") << polylinePrimitive->typeRuleIdIndex << xT(" (failed to resolve)") << std::endl;
                    output << xT("\t\tZ order: ") << polylinePrimitive->zOrder << std::endl;
                    output << xT("\t\tArea*2: ") << polylinePrimitive->doubledArea << std::endl;
                    for (const auto& evaluatedValueEntry : OsmAnd::rangeOf(OsmAnd::constOf(polylinePrimitive->evaluationResult.values)))
                    {
                        const auto valueDefinitionId = evaluatedValueEntry.key();
                        const auto value = evaluatedValueEntry.value();

                        const auto valueDefinition = mapStyle->getValueDefinitionById(valueDefinitionId);

                        output << xT("\t\t") << QStringToStlString(valueDefinition->name) << xT(" = ") << QStringToStlString(value.toString()) << std::endl;
                    }

                    polylinePrimitiveIndex++;
                }
            }

            if (!primitivisedGroup->polygons.isEmpty())
            {
                output << primitivisedGroup->polygons.size() << xT(" polygon(s):") << std::endl;

                unsigned int polygonPrimitiveIndex = 0u;
                for (const auto& polygonPrimitive : OsmAnd::constOf(primitivisedGroup->polygons))
                {
                    output << xT("\tPolygon #") << polygonPrimitiveIndex << std::endl;
                    QString ruleTag;
                    QString ruleValue;
                    const auto typeRuleResolved = primitivisedGroup->sourceObject->obtainTagValueByTypeRuleIndex(polygonPrimitive->typeRuleIdIndex, ruleTag, ruleValue);
                    if (typeRuleResolved)
                        output << xT("\t\tTag/value: ") << QStringToStlString(ruleTag) << xT(" = ") << QStringToStlString(ruleValue) << std::endl;
                    else
                        output << xT("\t\tTag/value: ") << polygonPrimitive->typeRuleIdIndex << xT(" (failed to resolve)") << std::endl;
                    output << xT("\t\tZ order: ") << polygonPrimitive->zOrder << std::endl;
                    output << xT("\t\tArea*2: ") << polygonPrimitive->doubledArea << std::endl;
                    for (const auto& evaluatedValueEntry : OsmAnd::rangeOf(OsmAnd::constOf(polygonPrimitive->evaluationResult.values)))
                    {
                        const auto valueDefinitionId = evaluatedValueEntry.key();
                        const auto value = evaluatedValueEntry.value();

                        const auto valueDefinition = mapStyle->getValueDefinitionById(valueDefinitionId);

                        output << xT("\t\t") << QStringToStlString(valueDefinition->name) << xT(" = ") << QStringToStlString(value.toString()) << std::endl;
                    }

                    polygonPrimitiveIndex++;
                }
            }
        }

        break;
    }

    return success;
}

bool OsmAndTools::Styler::evaluate(EvaluatedMapObjects& outEvaluatedMapObjects, QString *pLog /*= nullptr*/)
{
    if (pLog != nullptr)
    {
#if defined(_UNICODE) || defined(UNICODE)
        std::wostringstream output;
        const bool success = evaluate(outEvaluatedMapObjects, output);
        *pLog = QString::fromStdWString(output.str());
        return success;
#else
        std::ostringstream output;
        const bool success = evaluate(outEvaluatedMapObjects, output);
        *pLog = QString::fromStdString(output.str());
        return success;
#endif
    }
    else
    {
#if defined(_UNICODE) || defined(UNICODE)
        return evaluate(outEvaluatedMapObjects, std::wcout);
#else
        return evaluate(outEvaluatedMapObjects, std::cout);
#endif
    }
}

OsmAndTools::Styler::Configuration::Configuration()
    : styleName(QLatin1String("default"))
    , zoom(OsmAnd::ZoomLevel15)
    , displayDensityFactor(1.0f)
    , locale(QLatin1String("en"))
    , excludeCoastlines(false)
    , verbose(false)
{
}

bool OsmAndTools::Styler::Configuration::parseFromCommandLineArguments(
    const QStringList& commandLineArgs,
    Configuration& outConfiguration,
    QString& outError)
{
    outConfiguration = Configuration();

    const std::shared_ptr<OsmAnd::ObfsCollection> obfsCollection(new OsmAnd::ObfsCollection());
    outConfiguration.obfsCollection = obfsCollection;

    const std::shared_ptr<OsmAnd::MapStylesCollection> stylesCollection(new OsmAnd::MapStylesCollection());
    outConfiguration.stylesCollection = stylesCollection;

    for (const auto& arg : commandLineArgs)
    {
        if (arg.startsWith(QLatin1String("-obfsPath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-obfsPath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            obfsCollection->addDirectory(value, false);
        }
        else if (arg.startsWith(QLatin1String("-obfsRecursivePath=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-obfsRecursivePath=")));
            if (!QDir(value).exists())
            {
                outError = QString("'%1' path does not exist").arg(value);
                return false;
            }

            obfsCollection->addDirectory(value, true);
        }
        else if (arg.startsWith(QLatin1String("-obfFile=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-obfFile=")));
            if (!QFile(value).exists())
            {
                outError = QString("'%1' file does not exist").arg(value);
                return false;
            }

            obfsCollection->addFile(value);
        }
        else if (arg.startsWith(QLatin1String("-stylesPath=")))
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
        else if (arg.startsWith(QLatin1String("-styleSetting:")))
        {
            const auto settingValue = arg.mid(strlen("-styleSetting:"));
            const auto settingKeyValue = settingValue.split(QLatin1Char('='));
            if (settingKeyValue.size() != 2)
            {
                outError = QString("'%1' can not be parsed as style settings key and value").arg(settingValue);
                return false;
            }

            outConfiguration.styleSettings[settingKeyValue[0]] = Utilities::purifyArgumentValue(settingKeyValue[1]);
        }
        else if (arg.startsWith(QLatin1String("-mapObject=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-mapObject=")));

            uint64_t mapObjectId = 0u;
            bool ok = false;
            if (value[0] == QChar('-'))
                mapObjectId = static_cast<uint64_t>(value.toLongLong(&ok));
            else
                mapObjectId = value.toULongLong(&ok);

            if (!ok)
            {
                outError = QString("'%1' can not be parsed as map object identifier").arg(value);
                return false;
            }

            outConfiguration.mapObjectsIds.insert(mapObjectId);
        }
        else if (arg.startsWith(QLatin1String("-zoom=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-zoom=")));

            bool ok = false;
            outConfiguration.zoom = static_cast<OsmAnd::ZoomLevel>(value.toUInt(&ok));
            if (!ok || outConfiguration.zoom < OsmAnd::MinZoomLevel || outConfiguration.zoom > OsmAnd::MaxZoomLevel)
            {
                outError = QString("'%1' can not be parsed as zoom").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-displayDensityFactor=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-displayDensityFactor=")));

            bool ok = false;
            outConfiguration.displayDensityFactor = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as display density factor").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-locale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-locale=")));

            outConfiguration.locale = value;
        }
        else if (arg == QLatin1String("-verbose"))
        {
            outConfiguration.verbose = true;
        }
        else if (arg == QLatin1String("-excludeCoastlines"))
        {
            outConfiguration.excludeCoastlines = true;
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
