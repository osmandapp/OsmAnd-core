#include "ObfRoutingSectionReader.h"
#include "ObfRoutingSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfRoutingSectionReader::ObfRoutingSectionReader()
{
}

OsmAnd::ObfRoutingSectionReader::~ObfRoutingSectionReader()
{
}

void OsmAnd::ObfRoutingSectionReader::loadRoads(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const RoutingDataLevel dataLevel,
    const AreaI* const bbox31 /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::Road> >* resultOut /*= nullptr*/,
    const FilterRoadsByIdFunction filterById /*= nullptr*/,
    const VisitorFunction visitor /*= nullptr*/,
    DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric /*= nullptr*/)
{
    ObfRoutingSectionReader_P::loadRoads(
        *reader->_p,
        section,
        dataLevel,
        bbox31,
        resultOut,
        filterById,
        visitor,
        cache,
        outReferencedCacheEntries,
        controller,
        metric);
}

OsmAnd::ObfRoutingSectionReader::DataBlock::DataBlock(
    const DataBlockId id_,
    const RoutingDataLevel dataLevel_,
    const AreaI area31_,
    const QList< std::shared_ptr<const OsmAnd::Road> >& roads_)
    : id(id_)
    , dataLevel(dataLevel_)
    , area31(area31_)
    , roads(roads_)
{
}

OsmAnd::ObfRoutingSectionReader::DataBlock::~DataBlock()
{
}

OsmAnd::ObfRoutingSectionReader::DataBlocksCache::DataBlocksCache()
{
}

OsmAnd::ObfRoutingSectionReader::DataBlocksCache::~DataBlocksCache()
{
}

bool OsmAnd::ObfRoutingSectionReader::DataBlocksCache::shouldCacheBlock(
    const DataBlockId id,
    const RoutingDataLevel dataLevel,
    const AreaI blockBBox31,
    const AreaI* const queryArea31 /*= nullptr*/) const
{
    return true;
}
