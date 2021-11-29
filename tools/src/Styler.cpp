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
#include <OsmAndCore/Data/ObfMapSectionInfo.h>
#include <OsmAndCore/Data/BinaryMapObject.h>
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <OsmAndCore/Map/MapStyleEvaluationResult.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QTextStream>
#include <OsmAndCore/restore_internal_warnings.h>

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
            }
            else
            {
                output
                    << "Failed to resolve style '"
                    << QStringToStlString(configuration.styleName)
                    << "' from collection:"
                    << std::endl;
            }

            success = false;
            break;
        }

        // Load all map objects
        const auto mapObjectsFilterById =
            [this]
            (const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section,
                const uint64_t mapObjectId,
                const OsmAnd::AreaI& bbox,
                const OsmAnd::ZoomLevel firstZoomLevel,
                const OsmAnd::ZoomLevel lastZoomLevel,
                const OsmAnd::ZoomLevel requestedZoom) -> bool
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
        QList< std::shared_ptr<const OsmAnd::BinaryMapObject> > mapObjects_;
        success = obfDataInterface->loadBinaryMapObjects(
            &mapObjects_,
            nullptr,
            configuration.zoom,
            nullptr,
            configuration.mapObjectsIds.isEmpty() ? OsmAnd::ObfMapSectionReader::FilterByIdFunction() : mapObjectsFilterById,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        const auto mapObjects = OsmAnd::copyAs< QList< std::shared_ptr<const OsmAnd::MapObject> > >(mapObjects_);
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
                << xT(", map scale ")
                << configuration.mapScale
                << xT(", symbols scale ")
                << configuration.symbolsScale
                << xT(" and locale '")
                << QStringToStlString(configuration.locale)
                << xT("'...") << std::endl;
        }
        const std::shared_ptr<OsmAnd::MapPresentationEnvironment> mapPresentationEnvironment(new OsmAnd::MapPresentationEnvironment(
            mapStyle,
            configuration.displayDensityFactor,
            configuration.mapScale,
            configuration.symbolsScale,
            configuration.locale));

        if (configuration.verbose)
            output << xT("Applying extra style settings to map presentation environment...") << std::endl;
        mapPresentationEnvironment->setSettings(configuration.styleSettings);

        // Create primitiviser
        const std::shared_ptr<OsmAnd::MapPrimitiviser> primitiviser(new OsmAnd::MapPrimitiviser(mapPresentationEnvironment));
        if (configuration.verbose)
            output << xT("Going to primitivise map objects...") << std::endl;
        OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects metrics;
        const auto primitivisedData = primitiviser->primitiviseAllMapObjects(
            configuration.zoom,
            mapObjects,
            nullptr,
            nullptr,
            configuration.metrics ? &metrics : nullptr);
        if (configuration.verbose)
        {
            output
                << xT("Primitivised ")
                << primitivisedData->primitivesGroups.size()
                << xT(" groups from ")
                << mapObjects.size()
                << xT(" map objects") << std::endl;
        }

        // Obtain evaluated values for each group and print it
        for (const auto& primitivisedGroup : OsmAnd::constOf(primitivisedData->primitivesGroups))
        {
            const auto& mapObject = primitivisedGroup->sourceObject;
            const auto binaryMapObject = std::dynamic_pointer_cast<const OsmAnd::BinaryMapObject>(mapObject);
            const auto& attributeMapping = mapObject->attributeMapping;

            // Skip objects that were not requested
            if (!configuration.mapObjectsIds.isEmpty() &&
                binaryMapObject &&
                !configuration.mapObjectsIds.contains(binaryMapObject->id))
            {
                continue;
            }

            outEvaluatedMapObjects[mapObject] = primitivisedGroup;

            output << QStringToStlString(QString(80, QLatin1Char('-'))) << std::endl;
            output << QStringToStlString(mapObject->toString()) << std::endl;

            for (const auto& attributeId : OsmAnd::constOf(mapObject->attributeIds))
            {
                const auto attribute = attributeMapping->decodeMap.getRef(attributeId);
                if (attribute)
                {
                    output
                        << xT("\tType: ")
                        << QStringToStlString(attribute->tag)
                        << xT(" = ")
                        << QStringToStlString(attribute->value)
                        << std::endl;
                }
                else
                {
                    output << xT("\tType: #") << attributeId << xT(" (UNRESOLVED)") << std::endl;
                }
            }

            for (const auto& attributeId : OsmAnd::constOf(mapObject->additionalAttributeIds))
            {
                const auto attribute = attributeMapping->decodeMap.getRef(attributeId);
                if (attribute)
                {
                    output
                        << xT("\tExtra type: ")
                        << QStringToStlString(attribute->tag)
                        << xT(" = ")
                        << QStringToStlString(attribute->value)
                        << std::endl;
                }
                else
                {
                    output << xT("\tExtra type: #") << attributeId << xT(" (UNRESOLVED)") << std::endl;
                }
            }

            for (const auto& captionAttributeId : OsmAnd::constOf(mapObject->captionsOrder))
            {
                const auto& captionValue = mapObject->captions[captionAttributeId];

                if (attributeMapping->nativeNameAttributeId == captionAttributeId)
                    output << xT("\tCaption: ") << QStringToStlString(captionValue) << std::endl;
                else if (attributeMapping->refAttributeId == captionAttributeId)
                    output << xT("\tRef: ") << QStringToStlString(captionValue) << std::endl;
                else
                {
                    const auto itCaptionTagAsLocalizedName = attributeMapping->localizedNameAttributeIds.constFind(captionAttributeId);
                    const auto attribute = attributeMapping->decodeMap.getRef(captionAttributeId);

                    QString captionTag(QLatin1String("UNRESOLVED"));
                    if (itCaptionTagAsLocalizedName != attributeMapping->localizedNameAttributeIds.cend())
                        captionTag = itCaptionTagAsLocalizedName->toString();
                    if (attribute)
                        captionTag = attribute->tag;
                    output
                        << xT("\tCaption [")
                        << QStringToStlString(captionTag)
                        << xT("]: ")
                        << QStringToStlString(captionValue)
                        << std::endl;
                }
            }

            if (!primitivisedGroup->points.isEmpty())
            {
                output << primitivisedGroup->points.size() << xT(" point(s):") << std::endl;

                unsigned int pointPrimitiveIndex = 0u;
                for (const auto& pointPrimitive : OsmAnd::constOf(primitivisedGroup->points))
                {
                    output << xT("\tPoint #") << pointPrimitiveIndex << std::endl;
                    const auto attribute = mapObject->resolveAttributeByIndex(pointPrimitive->attributeIdIndex);
                    if (attribute)
                    {
                        output
                            << xT("\t\tAttribute: ")
                            << QStringToStlString(attribute->tag)
                            << xT(" = ")
                            << QStringToStlString(attribute->value)
                            << std::endl;
                    }
                    else
                    {
                        output
                            << xT("\t\tAttribute: ")
                            << pointPrimitive->attributeIdIndex
                            << xT(" (failed to resolve)")
                            << std::endl;
                    }
                    output << xT("\t\tZ order: ") << pointPrimitive->zOrder << std::endl;
                    output << xT("\t\tArea*2: ") << pointPrimitive->doubledArea << std::endl;
                    const auto& values = pointPrimitive->evaluationResult.getValues();
                    for (const auto& evaluatedValueEntry : OsmAnd::rangeOf(values))
                    {
                        const auto valueDefinitionId = evaluatedValueEntry.key();
                        const auto value = evaluatedValueEntry.value();

                        const auto& valueDefinition = mapStyle->getValueDefinitionById(valueDefinitionId);

                        output << xT("\t\t") << QStringToStlString(valueDefinition->name) << xT(" = ");
                        if (valueDefinition->dataType == OsmAnd::MapStyleValueDataType::Color)
                            output << QStringToStlString(OsmAnd::ColorARGB(value.toUInt()).toString());
                        else
                            output << QStringToStlString(value.toString());
                        output << std::endl;
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
                    const auto attribute = mapObject->resolveAttributeByIndex(polylinePrimitive->attributeIdIndex);
                    if (attribute)
                    {
                        output
                            << xT("\t\tAttribute: ")
                            << QStringToStlString(attribute->tag)
                            << xT(" = ")
                            << QStringToStlString(attribute->value) << std::endl;
                    }
                    else
                    {
                        output
                            << xT("\t\tAttribute: ")
                            << polylinePrimitive->attributeIdIndex
                            << xT(" (failed to resolve)")
                            << std::endl;
                    }
                    output << xT("\t\tZ order: ") << polylinePrimitive->zOrder << std::endl;
                    output << xT("\t\tArea*2: ") << polylinePrimitive->doubledArea << std::endl;
                    const auto& values = polylinePrimitive->evaluationResult.getValues();
                    for (const auto& evaluatedValueEntry : OsmAnd::rangeOf(values))
                    {
                        const auto valueDefinitionId = evaluatedValueEntry.key();
                        const auto value = evaluatedValueEntry.value();

                        const auto& valueDefinition = mapStyle->getValueDefinitionById(valueDefinitionId);

                        output << xT("\t\t") << QStringToStlString(valueDefinition->name) << xT(" = ");
                        if (valueDefinition->dataType == OsmAnd::MapStyleValueDataType::Color)
                            output << QStringToStlString(OsmAnd::ColorARGB(value.toUInt()).toString());
                        else
                            output << QStringToStlString(value.toString());
                        output << std::endl;
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
                    const auto attribute = mapObject->resolveAttributeByIndex(polygonPrimitive->attributeIdIndex);
                    if (attribute)
                    {
                        output
                            << xT("\t\tAttribute: ")
                            << QStringToStlString(attribute->tag)
                            << xT(" = ")
                            << QStringToStlString(attribute->value)
                            << std::endl;
                    }
                    else
                    {
                        output
                            << xT("\t\tAttribute: ")
                            << polygonPrimitive->attributeIdIndex
                            << xT(" (failed to resolve)")
                            << std::endl;
                    }
                    output << xT("\t\tZ order: ") << polygonPrimitive->zOrder << std::endl;
                    output << xT("\t\tArea*2: ") << polygonPrimitive->doubledArea << std::endl;
                    const auto& values = polygonPrimitive->evaluationResult.getValues();
                    for (const auto& evaluatedValueEntry : OsmAnd::rangeOf(values))
                    {
                        const auto valueDefinitionId = evaluatedValueEntry.key();
                        const auto value = evaluatedValueEntry.value();

                        const auto& valueDefinition = mapStyle->getValueDefinitionById(valueDefinitionId);

                        output << xT("\t\t") << QStringToStlString(valueDefinition->name) << xT(" = ");
                        if (valueDefinition->dataType == OsmAnd::MapStyleValueDataType::Color)
                            output << QStringToStlString(OsmAnd::ColorARGB(value.toUInt()).toString());
                        else
                            output << QStringToStlString(value.toString());
                        output << std::endl;
                    }

                    polygonPrimitiveIndex++;
                }
            }

            const auto itSymbolsGroup = primitivisedData->symbolsGroups.constFind(mapObject);
            if (itSymbolsGroup != primitivisedData->symbolsGroups.cend())
            {
                const auto& symbolsGroup = *itSymbolsGroup;

                output << symbolsGroup->symbols.size() << xT(" symbol(s):") << std::endl;

                unsigned int symbolIndex = 0u;
                for (const auto& symbol : OsmAnd::constOf(symbolsGroup->symbols))
                {
                    const auto textSymbol = std::dynamic_pointer_cast<const OsmAnd::MapPrimitiviser::TextSymbol>(symbol);
                    const auto iconSymbol = std::dynamic_pointer_cast<const OsmAnd::MapPrimitiviser::IconSymbol>(symbol);

                    output << xT("\tSymbol #") << symbolIndex;
                    if (textSymbol)
                        output << xT(" (text)");
                    else if (iconSymbol)
                        output << xT(" (icon)");
                    output << std::endl;

                    auto primitiveIndex = -1;
                    if (primitiveIndex == -1)
                    {
                        primitiveIndex = primitivisedGroup->points.indexOf(symbol->primitive);
                        if (primitiveIndex >= 0)
                            output << xT("\t\tPrimitive: Point #") << primitiveIndex << std::endl;
                    }
                    if (primitiveIndex == -1)
                    {
                        primitiveIndex = primitivisedGroup->polylines.indexOf(symbol->primitive);
                        if (primitiveIndex >= 0)
                            output << xT("\t\tPrimitive: Polyline #") << primitiveIndex << std::endl;
                    }
                    if (primitiveIndex == -1)
                    {
                        primitiveIndex = primitivisedGroup->polygons.indexOf(symbol->primitive);
                        if (primitiveIndex >= 0)
                            output << xT("\t\tPrimitive: Polygon #") << primitiveIndex << std::endl;
                    }

                    output << xT("\t\tPosition31: ") << symbol->location31.x << xT("x") << symbol->location31.y << std::endl;
                    output << xT("\t\tOrder: ") << symbol->order << std::endl;
                    output << xT("\t\tDraw along path: ") << (symbol->drawAlongPath ? xT("yes") : xT("no")) << std::endl;
                    output
                        << xT("\t\tIntersects with: ")
                        << QStringToStlString(QStringList(symbol->intersectsWith.values()).join(QLatin1String(", ")))
                        << std::endl;
                    output << xT("\t\tMinDistance: ") << symbol->minDistance << std::endl;
                    if (textSymbol)
                    {
                        output << xT("\t\tText: ") << QStringToStlString(textSymbol->value) << std::endl;
                        output << xT("\t\tLanguage: ");
                        switch (textSymbol->languageId)
                        {
                            case OsmAnd::LanguageId::Invariant:
                                output << xT("invariant");
                                break;
                            case OsmAnd::LanguageId::Native:
                                output << xT("native");
                                break;
                            case OsmAnd::LanguageId::Localized:
                                output << xT("localized");
                                break;
                        }
                        output << std::endl;
                        output << xT("\t\tDraw text on path: ") << (textSymbol->drawOnPath ? xT("yes") : xT("no")) << std::endl;
                        output << xT("\t\tText vertical offset: ") << textSymbol->verticalOffset << std::endl;
                        output << xT("\t\tText color: ") << QStringToStlString(textSymbol->color.toString()) << std::endl;
                        output << xT("\t\tText size: ") << textSymbol->size << std::endl;
                        output << xT("\t\tText shadow radius: ") << textSymbol->shadowRadius << std::endl;
                        output
                            << xT("\t\tText shadow color: ")
                            << QStringToStlString(textSymbol->shadowColor.toString())
                            << std::endl;
                        output << xT("\t\tText wrap width: ") << textSymbol->wrapWidth << std::endl;
                        output << xT("\t\tText is bold: ") << (textSymbol->isBold ? xT("yes") : xT("no")) << std::endl;
                        output << xT("\t\tText is italic: ") << (textSymbol->isItalic ? xT("yes") : xT("no")) << std::endl;
                        output
                            << xT("\t\tShield resource name: ")
                            << QStringToStlString(textSymbol->shieldResourceName)
                            << std::endl;
                        output
                            << xT("\t\tUnderlay icon name: ")
                            << QStringToStlString(textSymbol->underlayIconResourceName)
                            << std::endl;
                    }
                    else if (iconSymbol)
                    {
                        output << xT("\t\tIcon resource name: ") << QStringToStlString(iconSymbol->resourceName) << std::endl;
                        output
                            << xT("\t\tOverlay resource names: ")
                            << QStringToStlString(QStringList(iconSymbol->overlayResourceNames).join(QLatin1String(", ")))
                            << std::endl;
                        output
                            << xT("\t\tShield resource name: ")
                            << QStringToStlString(iconSymbol->shieldResourceName)
                            << std::endl;
                        output << xT("\t\tIntersection size: ") << iconSymbol->intersectionSize << std::endl;
                    }

                    symbolIndex++;
                }
            }
        }

        if (configuration.metrics)
        {
            output
                << xT("Metrics:\n")
                << QStringToStlString(metrics.toString(false, QLatin1String("\t")))
                << std::endl;
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
    , mapScale(1.0f)
    , symbolsScale(1.0f)
    , locale(QLatin1String("en"))
    , metrics(false)
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
        else if (arg.startsWith(QLatin1String("-mapScale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-mapScale=")));

            bool ok = false;
            outConfiguration.mapScale = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as map scale factor").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-symbolsScale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-symbolsScale=")));

            bool ok = false;
            outConfiguration.symbolsScale = value.toFloat(&ok);
            if (!ok)
            {
                outError = QString("'%1' can not be parsed as symbols scale factor").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-locale=")))
        {
            const auto value = Utilities::purifyArgumentValue(arg.mid(strlen("-locale=")));

            outConfiguration.locale = value;
        }
        else if (arg == QLatin1String("-metrics"))
        {
            outConfiguration.metrics = true;
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
    if (obfsCollection->getSourceOriginIds().isEmpty())
    {
        outError = QLatin1String("No OBF files found or specified");
        return false;
    }
    if (outConfiguration.styleName.isEmpty())
    {
        outError = QLatin1String("'styleName' can not be empty");
        return false;
    }

    return true;
}
