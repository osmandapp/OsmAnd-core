#include "OnlineRasterMapLayerProvider.h"
#include "OnlineRasterMapLayerProvider_P.h"

#include "QtExtensions.h"
#include <QStandardPaths>

OsmAnd::OnlineRasterMapLayerProvider::OnlineRasterMapLayerProvider(
    const QString& name_,
    const QString& urlPattern_,
    const ZoomLevel minZoom_ /*= MinZoomLevel*/,
    const ZoomLevel maxZoom_ /*= MaxZoomLevel*/,
    const uint32_t maxConcurrentDownloads_ /*= 1*/,
    const uint32_t providerTileSize_ /*= 256*/,
    const AlphaChannelData alphaChannelData_ /*= AlphaChannelData::Undefined*/)
    : _p(new OnlineRasterMapLayerProvider_P(this))
    , localCachePath(_p->_localCachePath)
    , networkAccessAllowed(_p->_networkAccessAllowed)
    , name(name_)
    , pathSuffix(QString(name).replace(QRegExp(QLatin1String("\\W+")), QLatin1String("_")))
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , providerTileSize(providerTileSize_)
    , alphaChannelData(alphaChannelData_)
{
    _p->_localCachePath = QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(pathSuffix));
}

OsmAnd::OnlineRasterMapLayerProvider::~OnlineRasterMapLayerProvider()
{
}

void OsmAnd::OnlineRasterMapLayerProvider::setLocalCachePath(const QDir& localCachePath, const bool appendPathSuffix /*= true*/)
{
    QMutexLocker scopedLocker(&_p->_localCachePathMutex);
    _p->_localCachePath = appendPathSuffix ? QDir(localCachePath.absoluteFilePath(pathSuffix)) : localCachePath;
}

void OsmAnd::OnlineRasterMapLayerProvider::setNetworkAccessPermission(bool allowed)
{
    _p->_networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineRasterMapLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    std::shared_ptr<OnlineRasterMapLayerProvider::Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, queryController);
    outTiledData = tiledData;

    return result;
}

float OsmAnd::OnlineRasterMapLayerProvider::getTileDensityFactor() const
{
    // Online tile providers do not have any idea about our tile density
    return 1.0f;
}

uint32_t OsmAnd::OnlineRasterMapLayerProvider::getTileSize() const
{
    return providerTileSize;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMaxZoom() const
{
    return maxZoom;
}
