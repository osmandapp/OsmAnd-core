#include "SlopeRasterMapLayerProvider.h"
#include "SlopeRasterMapLayerProvider_P.h"

#include "QtExtensions.h"
#include <QThreadPool>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"

#include <Logging.h>

OsmAnd::SlopeRasterMapLayerProvider::SlopeRasterMapLayerProvider(
    const std::shared_ptr<const IGeoTiffCollection>& filesCollection_,
    const QString& slopeColorsFilename,
    const uint32_t tileSize /*= 256*/,
    const float densityFactor /*= 1.0f*/)
    : _p(new SlopeRasterMapLayerProvider_P(this, slopeColorsFilename, tileSize, densityFactor))
    , filesCollection(filesCollection_)
    , _threadPool(new QThreadPool())
    , _lastRequestedZoom(ZoomLevel::ZoomLevel0)
    , _priority(0)
{
}

OsmAnd::SlopeRasterMapLayerProvider::~SlopeRasterMapLayerProvider()
{
     QMutexLocker scopedLocker(&_threadPoolMutex);
    _threadPool->clear();
    delete _threadPool;
}

OsmAnd::MapStubStyle OsmAnd::SlopeRasterMapLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

OsmAnd::ZoomLevel OsmAnd::SlopeRasterMapLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::SlopeRasterMapLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

uint32_t OsmAnd::SlopeRasterMapLayerProvider::getTileSize() const
{
    return _p->tileSize;
}

float OsmAnd::SlopeRasterMapLayerProvider::getTileDensityFactor() const
{
    return _p->densityFactor;
}

bool OsmAnd::SlopeRasterMapLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::SlopeRasterMapLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::SlopeRasterMapLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::SlopeRasterMapLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& r = MapDataProviderHelpers::castRequest<SlopeRasterMapLayerProvider::Request>(request);
    setLastRequestedZoom(r.zoom);

    const auto selfWeak = std::weak_ptr<SlopeRasterMapLayerProvider>(shared_from_this());
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
            const auto& r = MapDataProviderHelpers::castRequest<SlopeRasterMapLayerProvider::Request>(*requestClone);
            bool requestSucceeded = false;
            if (r.zoom == self->getLastRequestedZoom())
                requestSucceeded = self->obtainData(*requestClone, data, collectMetric ? &metric : nullptr);
            
            callback(self.get(), requestSucceeded, data, metric);
        }
    };
    
    const auto taskRunnable = new QRunnableFunctor(task);
    taskRunnable->setAutoDelete(true);
    int priority = getAndDecreasePriority();
    QMutexLocker scopedLocker(&_threadPoolMutex);
    if (_threadPool)
        _threadPool->start(taskRunnable, priority);
}

OsmAnd::ZoomLevel OsmAnd::SlopeRasterMapLayerProvider::getLastRequestedZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lastRequestedZoom;
}

void OsmAnd::SlopeRasterMapLayerProvider::setLastRequestedZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lastRequestedZoom != zoomLevel)
        _priority = 0;
    
    _lastRequestedZoom = zoomLevel;
}

int OsmAnd::SlopeRasterMapLayerProvider::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    _priority--;
    
    return _priority;
}
