#include "WeatherContourLayerProvider.h"

#include "MapDataProviderHelpers.h"
#include "WeatherTileResourceProvider.h"

OsmAnd::WeatherContourLayerProvider::WeatherContourLayerProvider(
    const std::shared_ptr<WeatherTileResourcesManager> resourcesManager,
    const QDateTime& dateTime,
    const QList<BandIndex> bands)
    : _resourcesManager(resourcesManager)
    , _dateTime(dateTime)
    , _bands(bands)
{
}

OsmAnd::WeatherContourLayerProvider::~WeatherContourLayerProvider()
{
}

const QDateTime OsmAnd::WeatherContourLayerProvider::getDateTime() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _dateTime;
}

void OsmAnd::WeatherContourLayerProvider::setDateTime(const QDateTime& dateTime)
{
    QWriteLocker scopedLocker(&_lock);
    
    _dateTime = dateTime;
}

const QList<OsmAnd::BandIndex> OsmAnd::WeatherContourLayerProvider::getBands() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _bands;
}

void OsmAnd::WeatherContourLayerProvider::setBands(const QList<BandIndex>& bands)
{
    QWriteLocker scopedLocker(&_lock);
    
    _bands = bands;
}

OsmAnd::MapStubStyle OsmAnd::WeatherContourLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

float OsmAnd::WeatherContourLayerProvider::getTileDensityFactor() const
{
    return _resourcesManager->getDensityFactor();
}

uint32_t OsmAnd::WeatherContourLayerProvider::getTileSize() const
{
    return _resourcesManager->getTileSize();
}

bool OsmAnd::WeatherContourLayerProvider::supportsNaturalObtainData() const
{
    return false;
}

bool OsmAnd::WeatherContourLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return false;
}

bool OsmAnd::WeatherContourLayerProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

void OsmAnd::WeatherContourLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request_,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& request = MapDataProviderHelpers::castRequest<IRasterMapLayerProvider::Request>(request_);
     
    WeatherTileResourcesManager::TileRequest _request;
    _request.weatherType = WeatherType::Contour;
    _request.dataTime = getDateTime();
    _request.tileId = request.tileId;
    _request.zoom = request.zoom;
    _request.bands = getBands();
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

OsmAnd::ZoomLevel OsmAnd::WeatherContourLayerProvider::getMinZoom() const
{
    return _resourcesManager->getMinTileZoom(WeatherType::Contour, WeatherLayer::High);
}

OsmAnd::ZoomLevel OsmAnd::WeatherContourLayerProvider::getMaxZoom() const
{
    return _resourcesManager->getMaxTileZoom(WeatherType::Contour, WeatherLayer::High);
}

OsmAnd::ZoomLevel OsmAnd::WeatherContourLayerProvider::getMinVisibleZoom() const
{
    return _resourcesManager->getMinTileZoom(WeatherType::Contour, WeatherLayer::High);
}

OsmAnd::ZoomLevel OsmAnd::WeatherContourLayerProvider::getMaxVisibleZoom() const
{
    return _resourcesManager->getMaxTileZoom(WeatherType::Contour, WeatherLayer::High);
}

int OsmAnd::WeatherContourLayerProvider::getMaxMissingDataZoomShift() const
{
    return _resourcesManager->getMaxMissingDataZoomShift(WeatherType::Contour, WeatherLayer::High);
}

int OsmAnd::WeatherContourLayerProvider::getMaxMissingDataUnderZoomShift() const
{
    return _resourcesManager->getMaxMissingDataUnderZoomShift(WeatherType::Contour, WeatherLayer::High);
}
