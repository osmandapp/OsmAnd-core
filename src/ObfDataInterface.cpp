#include "ObfDataInterface.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QSet>
#include "restore_internal_warnings.h"

#include "ObfReader.h"
#include "ObfInfo.h"
#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionReader.h"
#include "ObfRoutingSectionInfo.h"
#include "IQueryController.h"

OsmAnd::ObfDataInterface::ObfDataInterface(const QList< std::shared_ptr<const ObfReader> >& obfReaders_)
    : obfReaders(obfReaders_)
{
}

OsmAnd::ObfDataInterface::~ObfDataInterface()
{
}

bool OsmAnd::ObfDataInterface::loadObfFiles(QList< std::shared_ptr<const ObfFile> >* outFiles /*= nullptr*/, const IQueryController* const controller /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (controller && controller->isAborted())
            return false;

        // Initialize OBF file
        obfReader->obtainInfo();

        if (outFiles)
            outFiles->push_back(obfReader->obfFile);
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadBasemapPresenceFlag(bool& outBasemapPresent, const IQueryController* const controller /*= nullptr*/)
{
    outBasemapPresent = false;
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (controller && controller->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        outBasemapPresent = obfInfo->isBasemap;
        if (outBasemapPresent)
            break;
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadBinaryMapObjects(
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
    MapSurfaceType* outSurfaceType,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfMapSectionReader::FilterByIdFunction filterById /*= nullptr*/,
    ObfMapSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric /*= nullptr*/)
{
    auto mergedSurfaceType = MapSurfaceType::Undefined;
    std::shared_ptr<const ObfReader> basemapReader;

    for (const auto& obfReader : constOf(obfReaders))
    {
        if (controller && controller->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();

        // Handle basemap
        if (obfInfo->isBasemap)
        {
            // In case there's more than 1 basemap reader present, use only first and warn about this fact
            if (basemapReader)
            {
                LogPrintf(LogSeverityLevel::Warning, "More than 1 basemap available");
                continue;
            }

            // Save basemap reader for later use
            basemapReader = obfReader;

            // In case requested zoom is more detailed than basemap max zoom, skip basemap processing for now
            if (zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
                continue;
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (controller && controller->isAborted())
                return false;

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                obfReader,
                mapSection,
                zoom,
                bbox31,
                resultOut,
                &surfaceTypeToMerge,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                controller,
                metric);
            if (surfaceTypeToMerge != MapSurfaceType::Undefined)
            {
                if (mergedSurfaceType == MapSurfaceType::Undefined)
                    mergedSurfaceType = surfaceTypeToMerge;
                else if (mergedSurfaceType != surfaceTypeToMerge)
                    mergedSurfaceType = MapSurfaceType::Mixed;
            }
        }
    }

    // In case there's basemap available and requested zoom is more detailed than basemap max zoom level,
    // read tile from MaxBasemapZoomLevel that covers requested tile
    if (basemapReader && zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
    {
        const auto& obfInfo = basemapReader->obtainInfo();

        // Calculate proper bbox31 on MaxBasemapZoomLevel (if possible)
        const AreaI *pBasemapBBox31 = nullptr;
        AreaI basemapBBox31;
        if (bbox31)
        {
            pBasemapBBox31 = &basemapBBox31;
            basemapBBox31 = Utilities::roundBoundingBox31(*bbox31, static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel));
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (controller && controller->isAborted())
                return false;

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                basemapReader,
                mapSection,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel),
                pBasemapBBox31,
                resultOut,
                &surfaceTypeToMerge,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                controller,
                metric);

            // Basemap must always have a surface type defined
            assert(surfaceTypeToMerge != MapSurfaceType::Undefined);
            if (mergedSurfaceType == MapSurfaceType::Undefined)
                mergedSurfaceType = surfaceTypeToMerge;
            else if (mergedSurfaceType != surfaceTypeToMerge)
                mergedSurfaceType = MapSurfaceType::Mixed;
        }
    }

    // In case there was a basemap present, Undefined is Land
    if (mergedSurfaceType == MapSurfaceType::Undefined && !basemapReader)
        mergedSurfaceType = MapSurfaceType::FullLand;

    if (outSurfaceType)
        *outSurfaceType = mergedSurfaceType;

    return true;
}

bool OsmAnd::ObfDataInterface::loadRoads(
    const RoutingDataLevel dataLevel,
    const AreaI* const bbox31 /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::Road> >* resultOut /*= nullptr*/,
    const FilterRoadsByIdFunction filterById /*= nullptr*/,
    const ObfRoutingSectionReader::VisitorFunction visitor /*= nullptr*/,
    ObfRoutingSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (controller && controller->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& routingSection : constOf(obfInfo->routingSections))
        {
            if (controller && controller->isAborted())
                return false;

            OsmAnd::ObfRoutingSectionReader::loadRoads(
                obfReader,
                routingSection,
                dataLevel,
                bbox31,
                resultOut,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                controller,
                metric);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadMapObjects(
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* outBinaryMapObjects,
    QList< std::shared_ptr<const OsmAnd::Road> >* outRoads,
    MapSurfaceType* outSurfaceType,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfMapSectionReader::FilterByIdFunction filterMapObjectsById /*= nullptr*/,
    ObfMapSectionReader::DataBlocksCache* binaryMapObjectsCache /*= nullptr*/,
    QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedBinaryMapObjectsCacheEntries /*= nullptr*/,
    const FilterRoadsByIdFunction filterRoadsById /*= nullptr*/,
    ObfRoutingSectionReader::DataBlocksCache* roadsCache /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedRoadsCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const binaryMapObjectsMetric /*= nullptr*/,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const roadsMetric /*= nullptr*/)
{
    auto mergedSurfaceType = MapSurfaceType::Undefined;
    std::shared_ptr<const ObfReader> basemapReader;

    QSet<QString> processedMapSectionsNames;

    for (const auto& obfReader : constOf(obfReaders))
    {
        if (controller && controller->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();

        // Handle basemap
        if (obfInfo->isBasemap)
        {
            // In case there's more than 1 basemap reader present, use only first and warn about this fact
            if (basemapReader)
            {
                LogPrintf(LogSeverityLevel::Warning, "More than 1 basemap available");
                continue;
            }

            // Save basemap reader for later use
            basemapReader = obfReader;

            // In case requested zoom is more detailed than basemap max zoom, skip basemap processing for now
            if (zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
                continue;
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (controller && controller->isAborted())
                return false;

            // Remember that this section was processed (by name)
            processedMapSectionsNames.insert(mapSection->name);

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                obfReader,
                mapSection,
                zoom,
                bbox31,
                outBinaryMapObjects,
                &surfaceTypeToMerge,
                filterMapObjectsById,
                nullptr,
                binaryMapObjectsCache,
                outReferencedBinaryMapObjectsCacheEntries,
                controller,
                binaryMapObjectsMetric);
            if (surfaceTypeToMerge != MapSurfaceType::Undefined)
            {
                if (mergedSurfaceType == MapSurfaceType::Undefined)
                    mergedSurfaceType = surfaceTypeToMerge;
                else if (mergedSurfaceType != surfaceTypeToMerge)
                    mergedSurfaceType = MapSurfaceType::Mixed;
            }
        }
    }

    // In case there's basemap available and requested zoom is more detailed than basemap max zoom level,
    // read tile from MaxBasemapZoomLevel that covers requested tile
    if (basemapReader && zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
    {
        const auto& obfInfo = basemapReader->obtainInfo();

        // Calculate proper bbox31 on MaxBasemapZoomLevel (if possible)
        const AreaI *pBasemapBBox31 = nullptr;
        AreaI basemapBBox31;
        if (bbox31)
        {
            pBasemapBBox31 = &basemapBBox31;
            basemapBBox31 = Utilities::roundBoundingBox31(
                *bbox31,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel));
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (controller && controller->isAborted())
                return false;

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                basemapReader,
                mapSection,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel),
                pBasemapBBox31,
                outBinaryMapObjects,
                &surfaceTypeToMerge,
                filterMapObjectsById,
                nullptr,
                binaryMapObjectsCache,
                outReferencedBinaryMapObjectsCacheEntries,
                controller,
                binaryMapObjectsMetric);

            // Basemap must always have a surface type defined
            assert(surfaceTypeToMerge != MapSurfaceType::Undefined);
            if (mergedSurfaceType == MapSurfaceType::Undefined)
                mergedSurfaceType = surfaceTypeToMerge;
            else if (mergedSurfaceType != surfaceTypeToMerge)
                mergedSurfaceType = MapSurfaceType::Mixed;
        }
    }

    // In case there was a basemap present, Undefined is Land
    if (mergedSurfaceType == MapSurfaceType::Undefined && !basemapReader)
        mergedSurfaceType = MapSurfaceType::FullLand;

    if (outSurfaceType)
        *outSurfaceType = mergedSurfaceType;

    if (zoom > ObfMapSectionLevel::MaxBasemapZoomLevel)
    {
        for (const auto& obfReader : constOf(obfReaders))
        {
            if (controller && controller->isAborted())
                return false;

            const auto& obfInfo = obfReader->obtainInfo();

            // Skip all OBF readers that have map section, since they were already processed
            if (!obfInfo->mapSections.isEmpty())
                continue;

            for (const auto& routingSection : constOf(obfInfo->routingSections))
            {
                // Check if request is aborted
                if (controller && controller->isAborted())
                    return false;

                // Check that map section with same name was not processed from other file
                if (processedMapSectionsNames.contains(routingSection->name))
                    continue;

                // Read objects from each map section
                OsmAnd::ObfRoutingSectionReader::loadRoads(
                    obfReader,
                    routingSection,
                    RoutingDataLevel::Detailed,
                    bbox31,
                    outRoads,
                    filterRoadsById,
                    nullptr,
                    roadsCache,
                    outReferencedRoadsCacheEntries,
                    controller,
                    roadsMetric);
            }
        }
    }

    return true;
}
