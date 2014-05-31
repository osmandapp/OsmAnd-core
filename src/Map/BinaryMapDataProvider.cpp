#include "BinaryMapDataProvider.h"
#include "BinaryMapDataProvider_P.h"

#include "RasterizerEnvironment.h"
#include "RasterizerSharedContext.h"

OsmAnd::BinaryMapDataProvider::BinaryMapDataProvider(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const std::shared_ptr<const MapStyle>& mapStyle_,
    const float displayDensityFactor,
    const QString& localeLanguageId /*= QLatin1String("en")*/,
    const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider /*= nullptr*/ )
    : IMapTiledDataProvider(DataType::BinaryMapDataTile)
    , _p(new BinaryMapDataProvider_P(this))
    , obfsCollection(obfsCollection_)
    , mapStyle(mapStyle_)
    , rasterizerEnvironment(new RasterizerEnvironment(mapStyle_, displayDensityFactor, localeLanguageId, externalResourcesProvider))
    , rasterizerSharedContext(new RasterizerSharedContext())
{
}

OsmAnd::BinaryMapDataProvider::~BinaryMapDataProvider()
{
}

bool OsmAnd::BinaryMapDataProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<const MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

OsmAnd::BinaryMapDataTile::BinaryMapDataTile(
    const MapFoundationType tileFoundation_,
    const QList< std::shared_ptr<const Model::MapObject> >& mapObjects_,
    const std::shared_ptr< const RasterizerContext >& rasterizerContext_,
    const bool nothingToRasterize_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapTiledData(DataType::BinaryMapDataTile, tileId_, zoom_)
    , _p(new BinaryMapDataTile_P(this))
    , tileFoundation(tileFoundation_)
    , mapObjects(_p->_mapObjects)
    , rasterizerContext(_p->_rasterizerContext)
    , nothingToRasterize(nothingToRasterize_)
{
    _p->_mapObjects = mapObjects_;
    _p->_rasterizerContext = rasterizerContext_;
}

OsmAnd::BinaryMapDataTile::~BinaryMapDataTile()
{
    _p->cleanup();
}
