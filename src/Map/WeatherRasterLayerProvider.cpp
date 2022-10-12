#include "WeatherRasterLayerProvider.h"

#include "MapDataProviderHelpers.h"

OsmAnd::WeatherRasterLayerProvider::WeatherRasterLayerProvider(
    const std::shared_ptr<WeatherTileResourcesManager> resourcesManager,
    const WeatherLayer weatherLayer_,
    const int64_t dateTime,
    const QList<BandIndex> bands,
    const bool localData)
    : _resourcesManager(resourcesManager)
    , _dateTime(dateTime)
    , _bands(bands)
    , _localData(localData)
    , weatherLayer(weatherLayer_)
{
}

OsmAnd::WeatherRasterLayerProvider::~WeatherRasterLayerProvider()
{
}

const int64_t OsmAnd::WeatherRasterLayerProvider::getDateTime() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _dateTime;
}

void OsmAnd::WeatherRasterLayerProvider::setDateTime(int64_t dateTime)
{
    QWriteLocker scopedLocker(&_lock);
    
    _dateTime = dateTime;
}

const QList<OsmAnd::BandIndex> OsmAnd::WeatherRasterLayerProvider::getBands() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _bands;
}

void OsmAnd::WeatherRasterLayerProvider::setBands(const QList<BandIndex>& bands)
{
    QWriteLocker scopedLocker(&_lock);
    
    _bands = bands;
}

const bool OsmAnd::WeatherRasterLayerProvider::getLocalData() const
{
    QReadLocker scopedLocker(&_lock);

    return _localData;
}

void OsmAnd::WeatherRasterLayerProvider::setLocalData(const bool localData)
{
    QWriteLocker scopedLocker(&_lock);

    _localData = localData;
}

OsmAnd::MapStubStyle OsmAnd::WeatherRasterLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

float OsmAnd::WeatherRasterLayerProvider::getTileDensityFactor() const
{
    return _resourcesManager->getDensityFactor();
}

uint32_t OsmAnd::WeatherRasterLayerProvider::getTileSize() const
{
    return _resourcesManager->getTileSize();
}

bool OsmAnd::WeatherRasterLayerProvider::supportsNaturalObtainData() const
{
    return false;
}

bool OsmAnd::WeatherRasterLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return false;
}

bool OsmAnd::WeatherRasterLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::WeatherRasterLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request_,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& request = MapDataProviderHelpers::castRequest<IRasterMapLayerProvider::Request>(request_);
     
    WeatherTileResourcesManager::TileRequest _request;
    _request.weatherType = WeatherType::Raster;
    _request.weatherLayer = weatherLayer;
    _request.dateTime = getDateTime();
    _request.tileId = request.tileId;
    _request.zoom = request.zoom;
    _request.bands = getBands();
    _request.localData = getLocalData();
    _request.queryController = request.queryController;

    WeatherTileResourcesManager::ObtainTileDataAsyncCallback _callback =
        [this, callback]
        (const bool requestSucceeded,
            const std::shared_ptr<WeatherTileResourcesManager::Data>& data,
            const std::shared_ptr<Metric>& metric)
        {
            if (data)
            {
                const auto d = std::make_shared<IRasterMapLayerProvider::Data>(
                    data->tileId,
                    data->zoom,
                    data->alphaChannelPresence,
                    data->densityFactor,
                    data->image
                );
                callback(this, requestSucceeded, d, metric);
            }
            else
            {
                callback(this, false, nullptr, nullptr);
            }
        };
        
    _resourcesManager->obtainDataAsync(_request, _callback);
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMinZoom() const
{
    return _resourcesManager->getMinTileZoom(WeatherType::Raster, weatherLayer);
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMaxZoom() const
{
    return _resourcesManager->getMaxTileZoom(WeatherType::Raster, weatherLayer);
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMinVisibleZoom() const
{
    return _resourcesManager->getMinTileZoom(WeatherType::Raster, weatherLayer);
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMaxVisibleZoom() const
{
    return _resourcesManager->getMaxTileZoom(WeatherType::Raster, weatherLayer);
}

int OsmAnd::WeatherRasterLayerProvider::getMaxMissingDataZoomShift() const
{
    return _resourcesManager->getMaxMissingDataZoomShift(WeatherType::Raster, weatherLayer);
}

int OsmAnd::WeatherRasterLayerProvider::getMaxMissingDataUnderZoomShift() const
{
    return _resourcesManager->getMaxMissingDataUnderZoomShift(WeatherType::Raster, weatherLayer);
}
