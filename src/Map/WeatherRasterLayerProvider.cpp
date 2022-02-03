#include "WeatherRasterLayerProvider.h"
#include "WeatherRasterLayerProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"

OsmAnd::WeatherRasterLayerProvider::WeatherRasterLayerProvider(
    const QDateTime& dateTime_,
    const int bandIndex_,
    const QString& colorProfilePath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const QString& dataCachePath_ /*= QString()*/,
    const QString& projSearchPath_ /*= QString()*/,
    const std::shared_ptr<const IWebClient> webClient_ /*= nullptr*/)
    : _p(new WeatherRasterLayerProvider_P(this, dateTime_, bandIndex_, colorProfilePath_, tileSize_, densityFactor_, dataCachePath_, projSearchPath_, webClient_))
    , _threadPool(new QThreadPool())
    , dateTime(dateTime_)
    , bandIndex(bandIndex_)
    , colorProfilePath(colorProfilePath_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
    , dataCachePath(dataCachePath_)
    , projSearchPath(projSearchPath_)
{
}

OsmAnd::WeatherRasterLayerProvider::~WeatherRasterLayerProvider()
{
    _threadPool->clear();
    delete _threadPool;
}

OsmAnd::MapStubStyle OsmAnd::WeatherRasterLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

float OsmAnd::WeatherRasterLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::WeatherRasterLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::WeatherRasterLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::WeatherRasterLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::WeatherRasterLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::WeatherRasterLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& r = MapDataProviderHelpers::castRequest<WeatherRasterLayerProvider::Request>(request);
    setLastRequestedZoom(r.zoom);

    const auto selfWeak = std::weak_ptr<WeatherRasterLayerProvider>(shared_from_this());
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
            const auto& r = MapDataProviderHelpers::castRequest<WeatherRasterLayerProvider::Request>(*requestClone);
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

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getLastRequestedZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lastRequestedZoom;
}

void OsmAnd::WeatherRasterLayerProvider::setLastRequestedZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lastRequestedZoom != zoomLevel)
        _priority = 0;
    
    _lastRequestedZoom = zoomLevel;
}

int OsmAnd::WeatherRasterLayerProvider::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    _priority--;
    
    return _priority;
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMinVisibleZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMaxVisibleZoom() const
{
    return _p->getMaxZoom();
}
