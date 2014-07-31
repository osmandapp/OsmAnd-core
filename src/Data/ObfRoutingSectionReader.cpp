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
    QList< std::shared_ptr<const OsmAnd::Model::Road> >* resultOut /*= nullptr*/,
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
    const AreaI bbox31_,
    const QList< std::shared_ptr<const OsmAnd::Model::Road> >& roads_)
    : id(id_)
    , dataLevel(dataLevel_)
    , bbox31(bbox31_)
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

//void OsmAnd::ObfRoutingSectionReader::querySubsections(
//    const std::shared_ptr<ObfReader>& reader, const QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >& in,
//    QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut /*= nullptr*/, IQueryFilter* filter /*= nullptr*/,
//    std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor /*= nullptr*/ )
//{
//    ObfRoutingSectionReader_P::querySubsections(*reader->_p, in, resultOut, filter, visitor);
//}
//
//void OsmAnd::ObfRoutingSectionReader::loadSubsectionData(
//    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
//    QList< std::shared_ptr<const Model::Road> >* resultOut /*= nullptr*/, QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut /*= nullptr*/,
//    IQueryFilter* filter /*= nullptr*/, std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> visitor /*= nullptr*/ )
//{
//    ObfRoutingSectionReader_P::loadSubsectionData(*reader->_p, subsection, resultOut, resultMapOut, filter, visitor);
//}
//
//void OsmAnd::ObfRoutingSectionReader::loadSubsectionBorderBoxLinesPoints(
//    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
//    QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut /*= nullptr*/, IQueryFilter* filter /*= nullptr*/,
//    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine /*= nullptr*/,
//    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint /*= nullptr*/ )
//{
//    ObfRoutingSectionReader_P::loadSubsectionBorderBoxLinesPoints(*reader->_p, section, resultOut, filter, visitorLine);
//}
