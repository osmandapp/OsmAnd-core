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
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/Primitiviser.h>

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
bool OsmAndTools::Styler::evaluate(bool& outRejected, QHash<QString, QString>& outEvaluatedValues, std::wostream& output)
#else
bool OsmAndTools::Styler::evaluate(bool& outRejected, QHash<QString, QString>& outEvaluatedValues, std::ostream& output)
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
            const OsmAnd::ZoomLevel lasttZoomLevel) -> bool
            {
                return configuration.mapObjectsIds.contains(mapObjectId);
            };
        if (configuration.verbose)
            output << xT("Going to load ") << configuration.mapObjectsIds.size() << xT(" map objects...") << std::endl;
        const auto obfDataInterface = configuration.obfsCollection->obtainDataInterface();
        success = obfDataInterface->loadMapObjects(
            &mapObjects,
            nullptr,
            configuration.zoom,
            nullptr,
            mapObjectsFilterById,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (!success)
            break;


        //// Prepare all resources for renderer
        //if (configuration.verbose)
        //{
        //    output
        //        << xT("Initializing map presentation environment with display density ")
        //        << configuration.displayDensityFactor
        //        << xT(" and locale '")
        //        << QStringToStlString(configuration.locale)
        //        << xT("'...") << std::endl;
        //}
        //const std::shared_ptr<OsmAnd::MapPresentationEnvironment> mapPresentationEnvironment(new OsmAnd::MapPresentationEnvironment(
        //    mapStyle,
        //    configuration.displayDensityFactor,
        //    configuration.locale));

        //if (configuration.verbose)
        //    output << xT("Applying extra style settings to map presentation environment...") << std::endl;
        //mapPresentationEnvironment->setSettings(configuration.styleSettings);

        break;
    }

    return success;
}

bool OsmAndTools::Styler::evaluate(bool& outRejected, QHash<QString, QString>& outEvaluatedValues, QString *pLog /*= nullptr*/)
{
    if (pLog != nullptr)
    {
#if defined(_UNICODE) || defined(UNICODE)
        std::wostringstream output;
        const bool success = evaluate(outRejected, outEvaluatedValues, output);
        *pLog = QString::fromStdWString(output.str());
        return success;
#else
        std::ostringstream output;
        const bool success = evaluate(outRejected, outEvaluatedValues, output);
        *pLog = QString::fromStdString(output.str());
        return success;
#endif
    }
    else
    {
#if defined(_UNICODE) || defined(UNICODE)
        return evaluate(outRejected, outEvaluatedValues, std::wcout);
#else
        return evaluate(outRejected, outEvaluatedValues, std::cout);
#endif
    }
}

OsmAndTools::Styler::Configuration::Configuration()
    : styleName(QLatin1String("default"))
    , zoom(OsmAnd::ZoomLevel15)
    , displayDensityFactor(1.0f)
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
    if (outConfiguration.mapObjectsIds.isEmpty())
    {
        outError = QLatin1String("At least one 'mapObject' must be specified");
        return false;
    }

    return true;
}
