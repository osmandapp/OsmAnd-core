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
    const uint32_t tileSize_ /*= 256*/,
    const AlphaChannelPresence alphaChannelPresence_ /*= AlphaChannelPresence::Undefined*/,
    const float tileDensityFactor_ /*= 1.0f*/)
    : _p(new OnlineRasterMapLayerProvider_P(this))
    , localCachePath(_p->_localCachePath)
    , networkAccessAllowed(_p->_networkAccessAllowed)
    , name(name_)
    , pathSuffix(QString(name).replace(QRegExp(QLatin1String("\\W+")), QLatin1String("_")))
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , tileSize(tileSize_)
    , alphaChannelPresence(alphaChannelPresence_)
    , tileDensityFactor(tileDensityFactor_)
{
    _p->_localCachePath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(pathSuffix);
    if (_p->_localCachePath.isEmpty())
        _p->_localCachePath = QLatin1String(".");
}

OsmAnd::OnlineRasterMapLayerProvider::~OnlineRasterMapLayerProvider()
{
}

void OsmAnd::OnlineRasterMapLayerProvider::setLocalCachePath(
    const QString& localCachePath,
    const bool appendPathSuffix /*= true*/)
{
    QMutexLocker scopedLocker(&_p->_localCachePathMutex);
    _p->_localCachePath = appendPathSuffix
        ? QDir(localCachePath).absoluteFilePath(pathSuffix)
        : localCachePath;
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

OsmAnd::MapStubStyle OsmAnd::OnlineRasterMapLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

float OsmAnd::OnlineRasterMapLayerProvider::getTileDensityFactor() const
{
    return tileDensityFactor;
}

uint32_t OsmAnd::OnlineRasterMapLayerProvider::getTileSize() const
{
    return tileSize;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMaxZoom() const
{
    return maxZoom;
}

OsmAnd::IMapDataProvider::SourceType OsmAnd::OnlineRasterMapLayerProvider::getSourceType() const
{
    return IMapDataProvider::SourceType::NetworkDirect;
}
