#include "BinaryMapDataProvider.h"
#include "BinaryMapDataProvider_P.h"

#include "MapPresentationEnvironment.h"

OsmAnd::BinaryMapDataProvider::BinaryMapDataProvider(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : IMapTiledDataProvider(DataType::BinaryMapDataTile)
    , _p(new BinaryMapDataProvider_P(this))
    , obfsCollection(obfsCollection_)
{
}

OsmAnd::BinaryMapDataProvider::~BinaryMapDataProvider()
{
}

bool OsmAnd::BinaryMapDataProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, nullptr, queryController);
}

bool OsmAnd::BinaryMapDataProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    BinaryMapDataProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataProvider::getMinZoom() const
{
    return MinZoomLevel;//TODO: invalid
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataProvider::getMaxZoom() const
{
    return MaxZoomLevel;//TODO: invalid
}

OsmAnd::BinaryMapDataTile::BinaryMapDataTile(
    const std::shared_ptr<ObfMapSectionReader::DataBlocksCache>& dataBlocksCache_,
    const QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >& referencedDataBlocks_,
    const MapFoundationType tileFoundation_,
    const QList< std::shared_ptr<const Model::BinaryMapObject> >& mapObjects_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapTiledData(DataType::BinaryMapDataTile, tileId_, zoom_)
    , _p(new BinaryMapDataTile_P(this))
    , dataBlocksCache(dataBlocksCache_)
    , referencedDataBlocks(_p->_referencedDataBlocks)
    , tileFoundation(tileFoundation_)
    , mapObjects(_p->_mapObjects)
{
    _p->_referencedDataBlocks = referencedDataBlocks_;
    _p->_mapObjects = mapObjects_;
}

OsmAnd::BinaryMapDataTile::~BinaryMapDataTile()
{
    _p->cleanup();
}

void OsmAnd::BinaryMapDataTile::releaseConsumableContent()
{
    // There's no consumable data

    MapTiledData::releaseConsumableContent();
}
