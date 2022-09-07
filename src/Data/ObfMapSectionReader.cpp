#include "ObfMapSectionReader.h"
#include "ObfMapSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfMapSectionReader::ObfMapSectionReader()
{
}

OsmAnd::ObfMapSectionReader::~ObfMapSectionReader()
{
}

void OsmAnd::ObfMapSectionReader::loadMapObjects(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut /*= nullptr*/,
    MapSurfaceType* outBBoxOrSectionSurfaceType /*= nullptr*/,
    const ObfMapSectionReader::FilterByIdFunction filterById /*= nullptr*/,
    const VisitorFunction visitor /*= nullptr*/,
    DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric /*= nullptr*/,
    bool coastlineOnly /*= false*/)
{
    ObfMapSectionReader_P::loadMapObjects(
        *reader->_p,
        section,
        zoom,
        bbox31,
        resultOut,
        outBBoxOrSectionSurfaceType,
        filterById,
        visitor,
        cache,
        outReferencedCacheEntries,
        queryController,
        metric,
        coastlineOnly);
}

OsmAnd::ObfMapSectionReader::DataBlock::DataBlock(
    const DataBlockId id_,
    const AreaI bbox31_,
    const MapSurfaceType surfaceType_,
    const QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >& mapObjects_)
    : id(id_)
    , bbox31(bbox31_)
    , surfaceType(surfaceType_)
    , mapObjects(mapObjects_)
{
}

OsmAnd::ObfMapSectionReader::DataBlock::~DataBlock()
{
}

OsmAnd::ObfMapSectionReader::DataBlocksCache::DataBlocksCache()
{
}

OsmAnd::ObfMapSectionReader::DataBlocksCache::~DataBlocksCache()
{
}

bool OsmAnd::ObfMapSectionReader::DataBlocksCache::shouldCacheBlock(
    const DataBlockId id,
    const AreaI blockBBox31,
    const AreaI* const queryArea31 /*= nullptr*/) const
{
    return true;
}
