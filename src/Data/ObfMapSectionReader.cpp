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
    QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >* resultOut /*= nullptr*/,
    MapFoundationType* foundationOut /*= nullptr*/,
    const FilterMapObjectsByIdFunction filterById /*= nullptr*/,
    const VisitorFunction visitor /*= nullptr*/,
    DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric /*= nullptr*/)
{
    ObfMapSectionReader_P::loadMapObjects(
        *reader->_p,
        section,
        zoom,
        bbox31,
        resultOut,
        foundationOut,
        filterById,
        visitor,
        cache,
        outReferencedCacheEntries,
        controller,
        metric);
}

OsmAnd::ObfMapSectionReader::DataBlock::DataBlock(
    const DataBlockId id_,
    const AreaI bbox31_,
    const MapFoundationType foundationType_,
    const QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >& mapObjects_)
    : id(id_)
    , bbox31(bbox31_)
    , foundationType(foundationType_)
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
