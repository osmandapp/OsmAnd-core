#include "WeatherTileResourceProvider.h"
#include "WeatherTileResourceProvider_P.h"

#include "QtExtensions.h"
#include <QStandardPaths>
#include <QThreadPool>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "GeoTileRasterizer.h"
#include "QRunnableFunctor.h"

#include <Logging.h>

OsmAnd::WeatherTileResourceProvider::WeatherTileResourceProvider(
    const QDateTime& dateTime,
    const QHash<BandIndex, float>& bandOpacityMap,
    const QHash<BandIndex, QString>& bandColorProfilePaths,
    const QString& localCachePath,
    const QString& projResourcesPath,
    const uint32_t tileSize /*= 256*/,
    const float densityFactor /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new WeatherTileResourceProvider_P(this, dateTime, bandOpacityMap, bandColorProfilePaths, localCachePath, projResourcesPath, tileSize, densityFactor, webClient))
    , networkAccessAllowed(true)
{
}

OsmAnd::WeatherTileResourceProvider::~WeatherTileResourceProvider()
{
}

void OsmAnd::WeatherTileResourceProvider::obtainDataAsync(
    const TileRequest& request,
    const ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainDataAsync(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourceProvider::downloadGeoTilesAsync(
    const DownloadGeoTileRequest& request,
    const DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->downloadGeoTilesAsync(request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourceProvider::getGeoTileZoom()
{
    return ZoomLevel4;
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourceProvider::getTileZoom(const WeatherLayer layer)
{
    switch (layer)
    {
        case WeatherLayer::Low:
            return ZoomLevel4;
        case WeatherLayer::High:
            return ZoomLevel7;
        default:
            return ZoomLevel4;
    }
}

OsmAnd::WeatherLayer OsmAnd::WeatherTileResourceProvider::getWeatherLayerByZoom(const ZoomLevel zoom)
{
    
    int lowZoom = WeatherTileResourceProvider::getTileZoom(WeatherLayer::Low);
    int lowOverZoom = WeatherTileResourceProvider::getMaxMissingDataZoomShift(WeatherLayer::Low);
    int lowUnderZoom = WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(WeatherLayer::Low);
    if (zoom >= lowZoom - lowUnderZoom && zoom <= lowZoom + lowOverZoom)
        return WeatherLayer::Low;

    int highZoom = WeatherTileResourceProvider::getTileZoom(WeatherLayer::High);
    int highOverZoom = WeatherTileResourceProvider::getMaxMissingDataZoomShift(WeatherLayer::High);
    int highUnderZoom = WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(WeatherLayer::High);
    if (zoom >= highZoom - highUnderZoom && zoom <= highZoom + highOverZoom)
        return WeatherLayer::High;

    return WeatherLayer::Undefined;
}

int OsmAnd::WeatherTileResourceProvider::getMaxMissingDataZoomShift(const WeatherLayer layer)
{
    if (layer == WeatherLayer::Low)
        return 2;
    else if (layer == WeatherLayer::High)
        return 5;
    else
        return 0;
}

int OsmAnd::WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(const WeatherLayer layer)
{
    if (layer == WeatherLayer::Low)
        return 2;
    else if (layer == WeatherLayer::High)
        return 0;
    else
        return 0;
}

void OsmAnd::WeatherTileResourceProvider::setBandOpacityMap(const QHash<BandIndex, float>& bandOpacityMap)
{
    return _p->setBandOpacityMap(bandOpacityMap);
}

int OsmAnd::WeatherTileResourceProvider::getCurrentRequestVersion() const
{
    return _p->getCurrentRequestVersion();
}

bool OsmAnd::WeatherTileResourceProvider::closeProvider()
{
    return _p->closeProvider();
}

OsmAnd::WeatherTileResourceProvider::TileRequest::TileRequest()
    : tileId(TileId::zero())
    , zoom(InvalidZoomLevel)
    , version(0)
{
}

OsmAnd::WeatherTileResourceProvider::TileRequest::TileRequest(const TileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourceProvider::TileRequest::~TileRequest()
{
}

void OsmAnd::WeatherTileResourceProvider::TileRequest::copy(TileRequest& dst, const TileRequest& src)
{
    dst.tileId = src.tileId;
    dst.zoom = src.zoom;
    dst.bands = src.bands;
    dst.queryController = src.queryController;
    dst.version = src.version;
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider::TileRequest> OsmAnd::WeatherTileResourceProvider::TileRequest::clone() const
{
    return std::shared_ptr<TileRequest>(new TileRequest(*this));
}

OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::DownloadGeoTileRequest()
    : forceDownload(false)
{
}

OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::DownloadGeoTileRequest(const DownloadGeoTileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::~DownloadGeoTileRequest()
{
}

void OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::copy(DownloadGeoTileRequest& dst, const DownloadGeoTileRequest& src)
{
    dst.topLeft = src.topLeft;
    dst.bottomRight = src.bottomRight;
    dst.forceDownload = src.forceDownload;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest> OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::clone() const
{
    return std::shared_ptr<DownloadGeoTileRequest>(new DownloadGeoTileRequest(*this));
}

OsmAnd::WeatherTileResourceProvider::Data::Data(
    TileId tileId_,
    ZoomLevel zoom_,
    AlphaChannelPresence alphaChannelPresence_,
    float densityFactor_,
    sk_sp<const SkImage> image_)
    : tileId(tileId_)
    , zoom(zoom_)
    , alphaChannelPresence(alphaChannelPresence_)
    , densityFactor(densityFactor_)
    , image(image_)
{
}

OsmAnd::WeatherTileResourceProvider::Data::~Data()
{
}

