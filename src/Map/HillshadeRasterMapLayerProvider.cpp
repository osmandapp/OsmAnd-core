#include "HillshadeRasterMapLayerProvider.h"
#include "HillshadeRasterMapLayerProvider_P.h"

#include "QtExtensions.h"
#include <QThreadPool>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"

#include <Logging.h>

OsmAnd::HillshadeRasterMapLayerProvider::HillshadeRasterMapLayerProvider(
    const std::shared_ptr<const IGeoTiffCollection>& filesCollection_,
    const QString& hillshadeColorsFilename,
    const QString& slopeColorsFilename,
    const ZoomLevel minZoom /* ZoomLevel6 */,
    const ZoomLevel maxZoom /* ZoomLevel14 */,
    const uint32_t tileSize /* = 256 */,
    const float densityFactor /* = 1.0f */)
    : _p(new HillshadeRasterMapLayerProvider_P(this,
        hillshadeColorsFilename, slopeColorsFilename, minZoom, maxZoom, tileSize, densityFactor))
    , filesCollection(filesCollection_)
    , _threadPool(new QThreadPool())
    , _lastRequestedZoom(ZoomLevel::ZoomLevel0)
    , _priority(0)
{
}

OsmAnd::HillshadeRasterMapLayerProvider::~HillshadeRasterMapLayerProvider()
{
     QMutexLocker scopedLocker(&_threadPoolMutex);
    _threadPool->clear();
    delete _threadPool;
}

OsmAnd::MapStubStyle OsmAnd::HillshadeRasterMapLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

OsmAnd::ZoomLevel OsmAnd::HillshadeRasterMapLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::HillshadeRasterMapLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

uint32_t OsmAnd::HillshadeRasterMapLayerProvider::getTileSize() const
{
    return _p->tileSize;
}

float OsmAnd::HillshadeRasterMapLayerProvider::getTileDensityFactor() const
{
    return _p->densityFactor;
}

bool OsmAnd::HillshadeRasterMapLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::HillshadeRasterMapLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::HillshadeRasterMapLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::HillshadeRasterMapLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& r = MapDataProviderHelpers::castRequest<HillshadeRasterMapLayerProvider::Request>(request);
    setLastRequestedZoom(r.zoom);

    const auto selfWeak = std::weak_ptr<HillshadeRasterMapLayerProvider>(shared_from_this());
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
            const auto& r = MapDataProviderHelpers::castRequest<HillshadeRasterMapLayerProvider::Request>(*requestClone);
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

OsmAnd::ZoomLevel OsmAnd::HillshadeRasterMapLayerProvider::getLastRequestedZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lastRequestedZoom;
}

void OsmAnd::HillshadeRasterMapLayerProvider::setLastRequestedZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lastRequestedZoom != zoomLevel)
        _priority = 0;
    
    _lastRequestedZoom = zoomLevel;
}

int OsmAnd::HillshadeRasterMapLayerProvider::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    _priority--;
    
    return _priority;
}
