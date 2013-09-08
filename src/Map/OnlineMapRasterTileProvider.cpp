#include "OnlineMapRasterTileProvider.h"
#include "OnlineMapRasterTileProvider_P.h"

#include <assert.h>

#include "Logging.h"

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(
    const QString& id_,
    const QString& urlPattern_,
    const ZoomLevel& maxZoom_ /*= 31*/,
    const ZoomLevel& minZoom_ /*= 0*/,
    const uint32_t& maxConcurrentDownloads_ /*= 1*/,
    const uint32_t& tileSize_ /*= 256*/,
    const MapBitmapTile::AlphaChannelData& alphaChannelData_ /*= MapBitmapTile::AlphaChannelData::Undefined*/)
    : _d(new OnlineMapRasterTileProvider_P(this))
    , localCachePath(_d->_localCachePath)
    , networkAccessAllowed(_d->_networkAccessAllowed)
    , id(id_)
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , tileSize(tileSize_)
    , alphaChannelData(alphaChannelData_)
{
    _d->_localCachePath = QDir(QDir::current().filePath(id));
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const QDir& localCachePath )
{
    _d->_localCachePath = localCachePath;
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{
    _d->_networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTile( const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile )
{
    return _d->obtainTile(tileId, zoom, outTile);
}

float OsmAnd::OnlineMapRasterTileProvider::getTileDensity() const
{
    // Online tile providers do not have any idea about our tile density
    return 1.0f;
}

uint32_t OsmAnd::OnlineMapRasterTileProvider::getTileSize() const
{
    return tileSize;
}

std::shared_ptr<OsmAnd::IMapBitmapTileProvider> OsmAnd::OnlineMapRasterTileProvider::createMapnikProvider()
{
    auto provider = new OsmAnd::OnlineMapRasterTileProvider(
        "mapnik", "http://mapnik.osmand.net/${zoom}/${x}/${y}.png",
        ZoomLevel0, ZoomLevel18, 2,
        256, MapBitmapTile::AlphaChannelData::NotPresent);
    return std::shared_ptr<OsmAnd::IMapBitmapTileProvider>(static_cast<OsmAnd::IMapBitmapTileProvider*>(provider));
}

std::shared_ptr<OsmAnd::IMapBitmapTileProvider> OsmAnd::OnlineMapRasterTileProvider::createCycleMapProvider()
{
    auto provider = new OsmAnd::OnlineMapRasterTileProvider(
        "cyclemap", "http://b.tile.opencyclemap.org/cycle/${zoom}/${x}/${y}.png",
        ZoomLevel0, ZoomLevel18, 2,
        256, MapBitmapTile::AlphaChannelData::NotPresent);
    return std::shared_ptr<OsmAnd::IMapBitmapTileProvider>(static_cast<OsmAnd::IMapBitmapTileProvider*>(provider));
}
