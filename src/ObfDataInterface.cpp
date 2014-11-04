#include "ObfDataInterface.h"

#include "ObfReader.h"
#include "ObfInfo.h"
#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"
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

bool OsmAnd::ObfDataInterface::loadMapObjects(
    QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >* resultOut,
    MapFoundationType* outFoundation,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    const FilterMapObjectsByIdFunction filterById /*= nullptr*/,
    ObfMapSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric /*= nullptr*/)
{
    auto mergedFoundation = MapFoundationType::Undefined;
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
            auto foundationToMerge = MapFoundationType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                obfReader,
                mapSection,
                zoom,
                bbox31,
                resultOut,
                &foundationToMerge,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                controller,
                metric);
            if (foundationToMerge != MapFoundationType::Undefined)
            {
                if (mergedFoundation == MapFoundationType::Undefined)
                    mergedFoundation = foundationToMerge;
                else if (mergedFoundation != foundationToMerge)
                    mergedFoundation = MapFoundationType::Mixed;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //if (bbox31 && *bbox31 == Utilities::tileBoundingBox31(TileId::fromXY(4211, 2691), ZoomLevel13))
    //{
    //    const auto basemap = Utilities::tileBoundingBox31(TileId::fromXY(2105u >> 1, 1345u >> 1), ZoomLevel11);
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

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
            basemapBBox31 = *bbox31;

            const auto basemapTile31 = (1u << ObfMapSectionLevel::MaxBasemapZoomLevel);
            const auto subBasemapPrecisionMask = basemapTile31 - 1u;

            basemapBBox31.top() &= ~subBasemapPrecisionMask;
            basemapBBox31.left() &= ~subBasemapPrecisionMask;
            basemapBBox31.bottom() = (basemapBBox31.bottom() & (~subBasemapPrecisionMask)) + ((basemapBBox31.bottom() & subBasemapPrecisionMask) ? basemapTile31 : 0);
            basemapBBox31.right() = (basemapBBox31.right() & (~subBasemapPrecisionMask)) + ((basemapBBox31.right() & subBasemapPrecisionMask) ? basemapTile31 : 0);
            assert(basemapBBox31.contains(*bbox31));
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (controller && controller->isAborted())
                return false;

            // Read objects from each map section
            auto foundationToMerge = MapFoundationType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                basemapReader,
                mapSection,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel),
                pBasemapBBox31,
                resultOut,
                &foundationToMerge,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                controller,
                metric);

            // Basemap must always have a foundation defined
            assert(foundationToMerge != MapFoundationType::Undefined);
            if (mergedFoundation == MapFoundationType::Undefined)
                mergedFoundation = foundationToMerge;
            else if (mergedFoundation != foundationToMerge)
                mergedFoundation = MapFoundationType::Mixed;
        }
    }

    // In case there was a basemap present, Undefined is Land
    if (mergedFoundation == MapFoundationType::Undefined && !basemapReader)
        mergedFoundation = MapFoundationType::FullLand;

    if (outFoundation)
        *outFoundation = mergedFoundation;

    return true;
}

bool OsmAnd::ObfDataInterface::loadRoads(
    const RoutingDataLevel dataLevel,
    const AreaI* const bbox31 /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::Model::Road> >* resultOut /*= nullptr*/,
    const FilterRoadsByIdFunction filterById /*= nullptr*/,
    const ObfRoutingSectionReader::VisitorFunction visitor /*= nullptr*/,
    ObfRoutingSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric /*= nullptr*/)
{
    // Iterate through all OBF readers
    for (const auto& obfReader : constOf(obfReaders))
    {
        // Check if request is aborted
        if (controller && controller->isAborted())
            return false;

        // Iterate over all map sections of each OBF reader
        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& routingSection : constOf(obfInfo->routingSections))
        {
            // Check if request is aborted
            if (controller && controller->isAborted())
                return false;

            // Read objects from each map section
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
