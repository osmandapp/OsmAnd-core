#include "HeightRasterMapLayerProvider.h"
#include "HeightRasterMapLayerProvider_P.h"

#include "QtExtensions.h"
#include <QThreadPool>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"

#include <Logging.h>

OsmAnd::HeightRasterMapLayerProvider::HeightRasterMapLayerProvider(
    const std::shared_ptr<const IGeoTiffCollection>& filesCollection_,
    const QString& heightColorsFilename,
    const ZoomLevel minZoom /* ZoomLevel6 */,
    const ZoomLevel maxZoom /* ZoomLevel19 */,
    const uint32_t tileSize /* = 256 */,
    const float densityFactor /* = 1.0f */)
    : _p(new HeightRasterMapLayerProvider_P(this, heightColorsFilename, minZoom, maxZoom, tileSize, densityFactor))
    , filesCollection(filesCollection_)
    , _threadPool(new QThreadPool())
    , _lastRequestedZoom(ZoomLevel::ZoomLevel0)
    , _minVisibleZoom(minZoom)
    , _maxVisibleZoom(maxZoom)
    , _priority(0)
{
}

OsmAnd::HeightRasterMapLayerProvider::~HeightRasterMapLayerProvider()
{
     QMutexLocker scopedLocker(&_threadPoolMutex);
    _threadPool->clear();
    delete _threadPool;
}

OsmAnd::MapStubStyle OsmAnd::HeightRasterMapLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

OsmAnd::ZoomLevel OsmAnd::HeightRasterMapLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::HeightRasterMapLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

uint32_t OsmAnd::HeightRasterMapLayerProvider::getTileSize() const
{
    return _p->tileSize;
}

float OsmAnd::HeightRasterMapLayerProvider::getTileDensityFactor() const
{
    return _p->densityFactor;
}

bool OsmAnd::HeightRasterMapLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::HeightRasterMapLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::HeightRasterMapLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::HeightRasterMapLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& r = MapDataProviderHelpers::castRequest<HeightRasterMapLayerProvider::Request>(request);
    setLastRequestedZoom(r.zoom);

    const auto selfWeak = std::weak_ptr<HeightRasterMapLayerProvider>(shared_from_this());
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
            const auto& r = MapDataProviderHelpers::castRequest<HeightRasterMapLayerProvider::Request>(*requestClone);
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

OsmAnd::ZoomLevel OsmAnd::HeightRasterMapLayerProvider::getLastRequestedZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lastRequestedZoom;
}

void OsmAnd::HeightRasterMapLayerProvider::setLastRequestedZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lastRequestedZoom != zoomLevel)
        _priority = 0;
    
    _lastRequestedZoom = zoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::HeightRasterMapLayerProvider::getMinVisibleZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _minVisibleZoom;
}

OsmAnd::ZoomLevel OsmAnd::HeightRasterMapLayerProvider::getMaxVisibleZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _maxVisibleZoom;
}

void OsmAnd::HeightRasterMapLayerProvider::setMinVisibleZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    _minVisibleZoom = zoomLevel;
}

void OsmAnd::HeightRasterMapLayerProvider::setMaxVisibleZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    _maxVisibleZoom = zoomLevel;
}

int OsmAnd::HeightRasterMapLayerProvider::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    _priority--;
    
    return _priority;
}
