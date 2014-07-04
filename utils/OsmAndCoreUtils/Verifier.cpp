#include "Verifier.h"

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QFile>
#include <QStringList>
#include <QHash>

#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/ObfDataInterface.h>
#include <OsmAndCore/Data/ObfMapSectionInfo.h>

OsmAnd::Verifier::Configuration::Configuration()
    : action(Action::Invalid)
{
}

// Forward declarations
#if defined(_UNICODE) || defined(UNICODE)
void dump(std::wostream &output, const OsmAnd::Verifier::Configuration& cfg);
#else
void dump(std::ostream &output, const OsmAnd::Verifier::Configuration& cfg);
#endif

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::Verifier::dumpToStdOut( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    dump(std::wcout, cfg);
#else
    dump(std::cout, cfg);
#endif
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::Verifier::dumpToString( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    std::wostringstream output;
    dump(output, cfg);
    return QString::fromStdWString(output.str());
#else
    std::ostringstream output;
    dump(output, cfg);
    return QString::fromStdString(output.str());
#endif
}

OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL OsmAnd::Verifier::parseCommandLineArguments( const QStringList& cmdLineArgs, Configuration& cfg, QString& error )
{
    cfg.action = Configuration::Action::Invalid;

    for(auto itArg = cmdLineArgs.cbegin(); itArg != cmdLineArgs.cend(); ++itArg)
    {
        auto arg = *itArg;
        if(arg.startsWith("-obfsDir="))
            cfg.obfDirs.append(arg.mid(strlen("-obfsDir=")));
        if(arg.startsWith("-obf="))
            cfg.obfFiles.append(arg.mid(strlen("-obf=")));
        else if(arg == "-uniqueMapObjectIds")
            cfg.action = Configuration::Action::UniqueMapObjectIds;
    }

    if(cfg.obfDirs.isEmpty() && cfg.obfFiles.isEmpty())
    {
        error = "no input given";
        return false;
    }

    if(cfg.action == Configuration::Action::Invalid)
    {
        error = "no command given";
        return false;
    }

    return true;
}

#if defined(_UNICODE) || defined(UNICODE)
void dump(std::wostream &output, const OsmAnd::Verifier::Configuration& cfg)
#else
void dump(std::ostream &output, const OsmAnd::Verifier::Configuration& cfg)
#endif
{
    OsmAnd::ObfsCollection obfsCollection;
    for(const auto& obfsDir : cfg.obfDirs)
        obfsCollection.addDirectory(obfsDir);
    for(const auto& obfFile : cfg.obfFiles)
        obfsCollection.addFile(obfFile);
    const auto dataInterface = obfsCollection.obtainDataInterface();

    const auto obfFiles = obfsCollection.getObfFiles();
    output << "Will work with these files:" << std::endl;
    for(const auto& obfFile : obfFiles)
        output << "\t" << qPrintable(obfFile->filePath) << std::endl;

    if(cfg.action == OsmAnd::Verifier::Configuration::Action::UniqueMapObjectIds)
    {
        const OsmAnd::AreaI entireWorld(0, 0, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max());

        unsigned int lastReportedCount = 0;
        unsigned int totalDuplicatesCount = 0;

        for(int zoomLevel = OsmAnd::MinZoomLevel; zoomLevel <= OsmAnd::MaxZoomLevel; zoomLevel++)
        {
            output << "Processing " << zoomLevel << " zoom level..." << std::endl;

            QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> > duplicateMapObjects;
            QHash<uint64_t, unsigned int> mapObjectIds;
            const auto idsCollector = [&mapObjectIds, &lastReportedCount, &output, &totalDuplicatesCount](
                const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section,
                const uint64_t mapObjectId,
                const OsmAnd::AreaI& bbox,
                const OsmAnd::ZoomLevel firstZoomLevel,
                const OsmAnd::ZoomLevel lasttZoomLevel) -> bool
            {
                const auto itId = mapObjectIds.find(mapObjectId);
                if(itId != mapObjectIds.end())
                {
                    // Increment duplicates counter
                    itId.value()++;
                    totalDuplicatesCount++;

                    return true;
                }

                // Insert new entry
                mapObjectIds.insert(mapObjectId, 0);

                if(mapObjectIds.size() - lastReportedCount == 10000)
                {
                    output << "\t... processed 10k map objects, found " << totalDuplicatesCount << " duplicate(s) so far ... " << std::endl;
                    lastReportedCount = mapObjectIds.size();
                }

                return false;
            };

            dataInterface->loadMapObjects(&duplicateMapObjects, nullptr, entireWorld, static_cast<OsmAnd::ZoomLevel>(zoomLevel), nullptr, idsCollector, nullptr);

            output << "\tProcessed " << mapObjectIds.size() << " map objects.";
            if(!mapObjectIds.isEmpty())
                output << " Found " << totalDuplicatesCount << " duplicate(s) :";
            output << std::endl;
            for(auto itEntry = mapObjectIds.cbegin(); itEntry != mapObjectIds.cend(); ++itEntry)
            {
                const auto mapObjectId = itEntry.key();
                const auto duplicatesCount = itEntry.value();
                if(duplicatesCount == 0)
                    continue;

                output << "\tMapObject ";
                if(static_cast<int64_t>(mapObjectId) < 0)
                {
                    int64_t originalId = -static_cast<int64_t>(static_cast<uint64_t>(-static_cast<int64_t>(mapObjectId)) & 0xFFFFFFFF);
                    output << originalId;
                }
                else
                {
                    output << mapObjectId;
                }
                output << " has " << duplicatesCount << " duplicate(s)." << std::endl;
            }
        }

        output << "Totally " << totalDuplicatesCount << " duplicate(s) across all zoom levels" << std::endl;
    }
}
