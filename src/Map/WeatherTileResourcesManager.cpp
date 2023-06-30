#include "WeatherTileResourcesManager.h"
#include "WeatherTileResourcesManager_P.h"

#include <Utilities.h>

#include "Logging.h"

OsmAnd::WeatherTileResourcesManager::WeatherTileResourcesManager(
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings,
    const QString& localCachePath,
    const QString& projResourcesPath,
    const uint32_t tileSize /*= 256*/,
    const float densityFactor /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new WeatherTileResourcesManager_P(this,
        bandSettings, localCachePath, projResourcesPath, tileSize, densityFactor, webClient))
    , networkAccessAllowed(true)
{
}

OsmAnd::WeatherTileResourcesManager::~WeatherTileResourcesManager()
{
}

const QHash<OsmAnd::BandIndex, std::shared_ptr<const OsmAnd::GeoBandSettings>> OsmAnd::WeatherTileResourcesManager::getBandSettings() const
{
    return _p->getBandSettings();
}

void OsmAnd::WeatherTileResourcesManager::setBandSettings(const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings)
{
    _p->setBandSettings(bandSettings);
}

double OsmAnd::WeatherTileResourcesManager::getConvertedBandValue(const BandIndex band, const double value) const
{
    return _p->getConvertedBandValue(band, value);
}

QString OsmAnd::WeatherTileResourcesManager::getFormattedBandValue(const BandIndex band, const double value, const bool precise) const
{
    return _p->getFormattedBandValue(band, value, precise);
}

QString OsmAnd::WeatherTileResourcesManager::getLocalCachePath() const
{
    return _p->localCachePath;
}

QString OsmAnd::WeatherTileResourcesManager::getProjResourcesPath() const
{
    return _p->projResourcesPath;
}

uint32_t OsmAnd::WeatherTileResourcesManager::getTileSize() const
{
    return _p->tileSize;
}

float OsmAnd::WeatherTileResourcesManager::getDensityFactor() const
{
    return _p->densityFactor;
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager::getGeoTileZoom() const
{
    return _p->getGeoTileZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager::getMinTileZoom(const WeatherType type, const WeatherLayer layer) const
{
    return _p->getMinTileZoom(type, layer);
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourcesManager::getMaxTileZoom(const WeatherType type, const WeatherLayer layer) const
{
    return _p->getMaxTileZoom(type, layer);
}

int OsmAnd::WeatherTileResourcesManager::getMaxMissingDataZoomShift(const WeatherType type, const WeatherLayer layer) const
{
    return _p->getMaxMissingDataZoomShift(type, layer);
}

int OsmAnd::WeatherTileResourcesManager::getMaxMissingDataUnderZoomShift(const WeatherType type, const WeatherLayer layer) const
{
    return _p->getMaxMissingDataUnderZoomShift(type, layer);
}

bool OsmAnd::WeatherTileResourcesManager::isDownloadingTiles(const int64_t dateTime)
{
    return _p->isDownloadingTiles(dateTime);
}

bool OsmAnd::WeatherTileResourcesManager::isEvaluatingTiles(const int64_t dateTime)
{
    return _p->isEvaluatingTiles(dateTime);
}

QList<OsmAnd::TileId> OsmAnd::WeatherTileResourcesManager::getCurrentDownloadingTileIds(const int64_t dateTime)
{
    return _p->getCurrentDownloadingTileIds(dateTime);
}

QList<OsmAnd::TileId> OsmAnd::WeatherTileResourcesManager::getCurrentEvaluatingTileIds(const int64_t dateTime)
{
    return _p->getCurrentEvaluatingTileIds(dateTime);
}

QVector<OsmAnd::TileId> OsmAnd::WeatherTileResourcesManager::generateGeoTileIds(
        const LatLon topLeft,
        const LatLon bottomRight,
        const ZoomLevel zoom)
{
    QVector<TileId> geoTileIds;

    const auto topLeftTileId = TileId::fromXY(
            Utilities::getTileNumberX(zoom, topLeft.longitude),
            Utilities::getTileNumberY(zoom, topLeft.latitude));

    const auto bottomRightTileId = TileId::fromXY(
            Utilities::getTileNumberX(zoom, bottomRight.longitude),
            Utilities::getTileNumberY(zoom, bottomRight.latitude));

    if (topLeftTileId == bottomRightTileId)
    {
        geoTileIds << topLeftTileId;
    }
    else
    {
        int maxTileValue = (1 << zoom) - 1;
        auto x1 = topLeftTileId.x;
        auto y1 = topLeftTileId.y;
        auto x2 = bottomRightTileId.x;
        auto y2 = bottomRightTileId.y;
        if (x2 < x1)
            x2 = maxTileValue + x2 + 1;
        if (y2 < y1)
            y2 = maxTileValue + y2 + 1;

        auto y = y1;
        for (int iy = y1; iy <= y2; iy++)
        {
            auto x = x1;
            for (int ix = x1; ix <= x2; ix++)
            {
                geoTileIds << TileId::fromXY(x, y);
                x = x == maxTileValue ? 0 : x + 1;
            }
            y = y == maxTileValue ? 0 : y + 1;
        }
    }

    return geoTileIds;
}

void OsmAnd::WeatherTileResourcesManager::obtainValue(
    const ValueRequest& request,
    const ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainValue(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourcesManager::obtainValueAsync(
    const ValueRequest& request,
    const ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainValueAsync(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourcesManager::obtainData(
    const TileRequest& request,
    const ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    bool accept = false;
    switch (request.weatherType)
    {
        case WeatherType::Raster:
            accept = request.zoom == getMinTileZoom(WeatherType::Raster, request.weatherLayer)
                || request.zoom == getMaxTileZoom(WeatherType::Raster, request.weatherLayer);
            break;
        case WeatherType::Contour:
            accept = request.zoom >= getMinTileZoom(WeatherType::Contour, request.weatherLayer)
                && request.zoom <= getMaxTileZoom(WeatherType::Contour, request.weatherLayer);
            break;
        default:
            accept = false;
            break;
    }
    
    if (!accept)
    {
        callback(false, nullptr, nullptr);
        return;
    }
    _p->obtainData(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourcesManager::obtainDataAsync(
    const TileRequest& request,
    const ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    bool accept = false;
    switch (request.weatherType)
    {
        case WeatherType::Raster:
            accept = request.zoom == getMinTileZoom(WeatherType::Raster, request.weatherLayer)
                || request.zoom == getMaxTileZoom(WeatherType::Raster, request.weatherLayer);
            break;
        case WeatherType::Contour:
            accept = request.zoom >= getMinTileZoom(WeatherType::Contour, request.weatherLayer)
                && request.zoom <= getMaxTileZoom(WeatherType::Contour, request.weatherLayer);
            break;
        default:
            accept = false;
            break;
    }
    
    if (!accept)
    {
        callback(false, nullptr, nullptr);
        return;
    }
    _p->obtainDataAsync(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourcesManager::downloadGeoTiles(
    const DownloadGeoTileRequest& request,
    const DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->downloadGeoTiles(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourcesManager::downloadGeoTilesAsync(
    const DownloadGeoTileRequest& request,
    const DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->downloadGeoTilesAsync(request, callback, collectMetric);
}

uint64_t OsmAnd::WeatherTileResourcesManager::calculateDbCacheSize(
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom)
{
    return _p->calculateDbCacheSize(tileIds, excludeTileIds, zoom);
}

bool OsmAnd::WeatherTileResourcesManager::clearDbCache(
    const QList<TileId>& tileIds,
    const QList<TileId>& excludeTileIds,
    const ZoomLevel zoom)
{
    return _p->clearDbCache(tileIds, excludeTileIds, zoom);
}

bool OsmAnd::WeatherTileResourcesManager::clearDbCache(int64_t beforeDateTime /*= 0*/)
{
    return _p->clearDbCache(beforeDateTime);
}

OsmAnd::WeatherTileResourcesManager::ValueRequest::ValueRequest()
    : point31(0, 0)
    , zoom(ZoomLevel::InvalidZoomLevel)
    , band(0)
    , localData(false)
{
}

OsmAnd::WeatherTileResourcesManager::ValueRequest::ValueRequest(const ValueRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourcesManager::ValueRequest::~ValueRequest()
{
}

void OsmAnd::WeatherTileResourcesManager::ValueRequest::copy(ValueRequest& dst, const ValueRequest& src)
{
    dst.dateTime = src.dateTime;
    dst.point31 = src.point31;
    dst.zoom = src.zoom;
    dst.band = src.band;
    dst.localData = src.localData;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourcesManager::ValueRequest> OsmAnd::WeatherTileResourcesManager::ValueRequest::clone() const
{
    return std::shared_ptr<WeatherTileResourcesManager::ValueRequest>(new ValueRequest(*this));
}

OsmAnd::WeatherTileResourcesManager::TileRequest::TileRequest()
    : weatherLayer(WeatherLayer::Undefined)
    , weatherType(WeatherType::Raster)
    , tileId(TileId::zero())
    , zoom(InvalidZoomLevel)
{
}

OsmAnd::WeatherTileResourcesManager::TileRequest::TileRequest(const TileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourcesManager::TileRequest::~TileRequest()
{
}

void OsmAnd::WeatherTileResourcesManager::TileRequest::copy(TileRequest& dst, const TileRequest& src)
{
    dst.weatherLayer = src.weatherLayer;
    dst.weatherType = src.weatherType;
    dst.dateTime = src.dateTime;
    dst.tileId = src.tileId;
    dst.zoom = src.zoom;
    dst.bands = src.bands;
    dst.localData = src.localData;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourcesManager::TileRequest> OsmAnd::WeatherTileResourcesManager::TileRequest::clone() const
{
    return std::shared_ptr<WeatherTileResourcesManager::TileRequest>(new TileRequest(*this));
}

OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::DownloadGeoTileRequest()
    : forceDownload(false)
    , localData(false)
{
}

OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::DownloadGeoTileRequest(const DownloadGeoTileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::~DownloadGeoTileRequest()
{
}

void OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::copy(DownloadGeoTileRequest& dst, const DownloadGeoTileRequest& src)
{
    dst.dateTime = src.dateTime;
    dst.topLeft = src.topLeft;
    dst.bottomRight = src.bottomRight;
    dst.forceDownload = src.forceDownload;
    dst.localData = src.localData;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest> OsmAnd::WeatherTileResourcesManager::DownloadGeoTileRequest::clone() const
{
    return std::shared_ptr<WeatherTileResourcesManager::DownloadGeoTileRequest>(new DownloadGeoTileRequest(*this));
}

OsmAnd::WeatherTileResourcesManager::Data::Data(
    TileId tileId_,
    ZoomLevel zoom_,
    AlphaChannelPresence alphaChannelPresence_,
    float densityFactor_,
    sk_sp<const SkImage> image_,
    QHash<BandIndex, QList<std::shared_ptr<GeoContour>>> contourMap_ /*= QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>()*/)
    : tileId(tileId_)
    , zoom(zoom_)
    , alphaChannelPresence(alphaChannelPresence_)
    , densityFactor(densityFactor_)
    , image(image_)
    , contourMap(contourMap_)
{
}

OsmAnd::WeatherTileResourcesManager::Data::~Data()
{
}
