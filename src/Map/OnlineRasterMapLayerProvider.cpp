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
    const std::shared_ptr<const IOnlineTileSources::Source> tileSource,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new OnlineRasterMapLayerProvider_P(this, webClient))
    , _threadPool(new QThreadPool())
    , _lastRequestedZoom(ZoomLevel0)
    , _priority(0)
    , name(tileSource->name)
    , pathSuffix(QString(name))
    , _tileSource(tileSource)
    , maxConcurrentDownloads(0)
    , alphaChannelPresence(AlphaChannelPresence::NotPresent)
    , tileDensityFactor(tileSource->bitDensity / 16.0)
    , localCachePath(_p->_localCachePath)
    , networkAccessAllowed(_p->_networkAccessAllowed)
{
    _p->_localCachePath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(pathSuffix);
    if (_p->_localCachePath.isEmpty())
        _p->_localCachePath = QLatin1String(".");
}

OsmAnd::OnlineRasterMapLayerProvider::~OnlineRasterMapLayerProvider()
{
    _threadPool->clear();
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
    return _tileSource->tileSize;
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
    const auto& r = MapDataProviderHelpers::castRequest<OnlineRasterMapLayerProvider::Request>(request);
    setLastRequestedZoom(r.zoom);

    const auto selfWeak = std::weak_ptr<OnlineRasterMapLayerProvider>(shared_from_this());
    const auto requestClone = request.clone();
    const QRunnableFunctor::Callback task =
    [selfWeak, requestClone, callback, collectMetric]
    (const QRunnableFunctor* const runnable)
    {
        const auto self = selfWeak.lock();
        if (self)
        {
            std::shared_ptr<IMapDataProvider::Data> data;
            std::shared_ptr<Metric> metric;
            const auto& r = MapDataProviderHelpers::castRequest<OnlineRasterMapLayerProvider::Request>(*requestClone);
            bool requestSucceeded = false;
            if (r.zoom == self->getLastRequestedZoom())
                requestSucceeded = self->obtainData(*requestClone, data, collectMetric ? &metric : nullptr);
            
            callback(self.get(), requestSucceeded, data, metric);
        }
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
    return _tileSource->minZoom;
}

OsmAnd::ZoomLevel OsmAnd::OnlineRasterMapLayerProvider::getMaxZoom() const
{
    return _tileSource->maxZoom;
}

const QString OsmAnd::OnlineRasterMapLayerProvider::buildUrlToLoad(const QString& urlToLoad, const QList<QString> randomsArray, int32_t x, int32_t y, const ZoomLevel zoom)
{
    return OnlineRasterMapLayerProvider_P::buildUrlToLoad(urlToLoad, randomsArray, x, y, zoom);
}
