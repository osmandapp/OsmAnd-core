#include "ObfDataInterface.h"

#include "ObfReader.h"
#include "ObfInfo.h"
#include "ObfMapSectionReader.h"
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
    MapFoundationType* foundationOut,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    const FilterMapObjectsByIdFunction filterById /*= nullptr*/,
    ObfMapSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric /*= nullptr*/)
{
    if (foundationOut)
        *foundationOut = MapFoundationType::Undefined;

    // Iterate through all OBF readers
    for (const auto& obfReader : constOf(obfReaders))
    {
        // Check if request is aborted
        if (controller && controller->isAborted())
            return false;

        // Iterate over all map sections of each OBF reader
        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            // Check if request is aborted
            if (controller && controller->isAborted())
                return false;

            // Read objects from each map section
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                obfReader,
                mapSection,
                zoom,
                bbox31,
                resultOut,
                foundationOut,
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
