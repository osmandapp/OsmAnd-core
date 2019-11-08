#include "OnlineRasterMapLayerProvider.h"
#include "OnlineRasterMapLayerProvider_P.h"

#include "QtExtensions.h"
#include <QStandardPaths>
#include <QThreadPool>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"

#include <Logging.h>

OsmAnd::OnlineRasterMapLayerProvider::OnlineRasterMapLayerProvider(
    const QString& name_,
    const QString& urlPattern_,
    const ZoomLevel minZoom_ /*= MinZoomLevel*/,
    const ZoomLevel maxZoom_ /*= MaxZoomLevel*/,
    const uint32_t maxConcurrentDownloads_ /*= 1*/,
    const uint32_t tileSize_ /*= 256*/,
    const AlphaChannelPresence alphaChannelPresence_ /*= AlphaChannelPresence::Undefined*/,
    const float tileDensityFactor_ /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new OnlineRasterMapLayerProvider_P(this, webClient))
    , _threadPool(new QThreadPool())
    , _lastRequestedZoom(ZoomLevel0)
    , _priority(0)
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
    _threadPool->clear();
    _threadPool->waitForDone();
    delete _threadPool;
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

bool OsmAnd::OnlineRasterMapLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::OnlineRasterMapLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::OnlineRasterMapLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::OnlineRasterMapLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    //MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
    
    const auto& r = MapDataProviderHelpers::castRequest<OnlineRasterMapLayerProvider::Request>(request);
    setLastRequestedZoom(r.zoom);

    const auto requestClone = request.clone();
    const QRunnableFunctor::Callback task =
    [this, requestClone, callback, collectMetric]
    (const QRunnableFunctor* const runnable)
    {
        std::shared_ptr<IMapDataProvider::Data> data;
        std::shared_ptr<Metric> metric;

        const auto& r = MapDataProviderHelpers::castRequest<OnlineRasterMapLayerProvider::Request>(*requestClone);
        bool requestSucceeded = false;
        if (r.zoom == getLastRequestedZoom())
            requestSucceeded = this->obtainData(*requestClone, data, collectMetric ? &metric : nullptr);

        callback(this, requestSucceeded, data, metric);
    };
    
    const auto taskRunnable = new QRunnableFunctor(task);
    taskRunnable->setAutoDelete(true);
    int priority = getAndDecreasePriority();
    _threadPool->start(taskRunnable, priority);
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getLastRequestedZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lastRequestedZoom;
}

void OsmAnd::OnlineRasterMapLayerProvider::setLastRequestedZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lastRequestedZoom != zoomLevel)
        _priority = 0;
    
    _lastRequestedZoom = zoomLevel;
}

int OsmAnd::OnlineRasterMapLayerProvider::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    _priority--;
    
    return _priority;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMaxZoom() const
{
    return maxZoom;
}
