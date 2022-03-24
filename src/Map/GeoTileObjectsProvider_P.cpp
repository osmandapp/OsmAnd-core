#include "GeoTileObjectsProvider_P.h"
#include "GeoTileObjectsProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapObject.h"
#include "Utilities.h"

OsmAnd::GeoTileObjectsProvider_P::GeoTileObjectsProvider_P(GeoTileObjectsProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::GeoTileObjectsProvider_P::~GeoTileObjectsProvider_P()
{
}

void OsmAnd::GeoTileObjectsProvider_P::addDataToCache(const std::shared_ptr<GeoTileObjectsProvider::Data>& data)
{
    auto cacheSize = owner->cacheSize;
    if (cacheSize == 0)
        return;
    
    QWriteLocker scopedLocker(&_dataCacheLock);

    _dataCache.push_back(data);
    
    while (_dataCache.size() > cacheSize)
        _dataCache.removeFirst();
}

OsmAnd::ZoomLevel OsmAnd::GeoTileObjectsProvider_P::getMinZoom() const
{
    return owner->_resourcesManager->getMinTileZoom(WeatherType::Contour, WeatherLayer::High);
}

OsmAnd::ZoomLevel OsmAnd::GeoTileObjectsProvider_P::getMaxZoom() const
{
    return owner->_resourcesManager->getMaxTileZoom(WeatherType::Contour, WeatherLayer::High);
}

bool OsmAnd::GeoTileObjectsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<GeoTileObjectsProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    std::shared_ptr<TileEntry> tileEntry;

    for (;;)
    {
        // Try to obtain previous instance of tile
        _tileReferences.obtainOrAllocateEntry(tileEntry, request.tileId, request.zoom,
            []
            (const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
            {
                return new TileEntry(collection, tileId, zoom);
            });

        // If state is "Undefined", change it to "Loading" and proceed with loading
        if (tileEntry->setStateIf(TileState::Undefined, TileState::Loading))
        {
            //LogPrintf(LogSeverityLevel::Debug, "!!! Retrieve weather contour tile %dx%d@%d", request.tileId.x, request.tileId.y, request.zoom);
            break;
        }

        // In case tile entry is being loaded, wait until it will finish loading
        if (tileEntry->getState() == TileState::Loading)
        {
            QReadLocker scopedLcoker(&tileEntry->loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while (tileEntry->getState() != TileState::Loaded)
                REPEAT_UNTIL(tileEntry->loadedCondition.wait(&tileEntry->loadedConditionLock));
        }

        if (!tileEntry->dataIsPresent)
        {
            // If there was no data, return same
            outData.reset();
            return true;
        }
        else
        {
            // Otherwise, try to lock tile reference
            outData = tileEntry->dataWeakRef.lock();

            // If successfully locked, just return it
            if (outData)
            {
                //LogPrintf(LogSeverityLevel::Debug, "!!! Reuse weather contour tile %dx%d@%d", request.tileId.x, request.tileId.y, request.zoom);
                return true;
            }

            //LogPrintf(LogSeverityLevel::Debug, "!!! Drop weather contour tile %dx%d@%d", request.tileId.x, request.tileId.y, request.zoom);

            // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
            // This will enable creation of new entry on next loop cycle
            _tileReferences.removeEntry(request.tileId, request.zoom);
            tileEntry.reset();
        }
    }

    bool result = false;
    
    auto settings = owner->_resourcesManager->getBandSettings().value(owner->band, nullptr);
    if (!settings)
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        outData.reset();
    }
    auto contourStyleName = settings->contourStyleName;
    auto contourTypes = settings->contourTypes.value(request.zoom);

    auto band = owner->band;
    QList<BandIndex> bands;
    bands << band;
        
    WeatherTileResourcesManager::TileRequest _request;
    _request.weatherType = WeatherType::Contour;
    _request.dataTime = owner->dateTime;
    _request.tileId = request.tileId;
    _request.zoom = request.zoom;
    _request.bands = bands;
    _request.queryController = request.queryController;

    WeatherTileResourcesManager::ObtainTileDataAsyncCallback _callback =
        [this, band, &contourStyleName, &contourTypes, &request, &outData, &tileEntry, &result]
        (const bool requestSucceeded,
            const std::shared_ptr<WeatherTileResourcesManager::Data>& data,
            const std::shared_ptr<Metric>& metric)
        {
            if (data && !data->contourMap.empty())
            {
                QList<std::shared_ptr<const MapObject>> mapObjects;
                
                const auto& contours = data->contourMap.values().first();
                for (auto& contour : contours)
                {
                    if (contour->points.empty())
                        continue;
                    
                    auto mapObj = std::make_shared<OsmAnd::MapObject>();
                    mapObj->points31 = contour->points;
                    //mapObj->computeBBox31();
                    mapObj->bbox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
                    
                    auto attributeMapping = std::make_shared<OsmAnd::MapObject::AttributeMapping>();
                    
                    int idx = 1;
                    mapObj->attributeIds.push_back(idx);
                    attributeMapping->registerMapping(idx++, QStringLiteral("contour"), contourStyleName);
                    attributeMapping->registerMapping(idx++, contourStyleName, QStringLiteral(""));
                    for (const auto& contourType : constOf(contourTypes))
                    {
                        mapObj->additionalAttributeIds.push_back(idx);
                        attributeMapping->registerMapping(idx++, QStringLiteral("contourtype"), contourType);
                    }
                    attributeMapping->verifyRequiredMappingRegistered();
                    mapObj->attributeMapping = attributeMapping;

                    auto value = owner->_resourcesManager->getConvertedBandValue(band, contour->value);
                    auto valueStr = owner->_resourcesManager->getFormattedBandValue(band, value, false);
                    mapObj->captions.insert(2, valueStr);
                    mapObj->captionsOrder << 2;

                    mapObjects << mapObj;

//                    LogPrintf(LogSeverityLevel::Debug, "Weather contour line: %.2f (%d) tile %dx%d@%d",
//                              contour->value, contour->points.length(), request.tileId.x, request.tileId.y, request.zoom);
                }

//                LogPrintf(LogSeverityLevel::Debug, "Weather contour line tile %dx%d@%d",
//                          request.tileId.x, request.tileId.y, request.zoom);

                const auto newTiledData = std::make_shared<GeoTileObjectsProvider::Data>(
                    request.tileId,
                    request.zoom,
                    MapSurfaceType::Undefined,
                    mapObjects);

                // Publish new tile
                outData = newTiledData;
                addDataToCache(newTiledData);

                // Store weak reference to new tile and mark it as 'Loaded'
                tileEntry->dataIsPresent = true;
                tileEntry->dataWeakRef = newTiledData;
                tileEntry->setState(TileState::Loaded);
            }
            else
            {
                // Create empty tile to avoid rendering artefacts while zooming
                QList<std::shared_ptr<const MapObject>> mapObjects;
                auto mapObj = std::make_shared<OsmAnd::MapObject>();
                mapObj->bbox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
                auto attributeMapping = std::make_shared<OsmAnd::MapObject::AttributeMapping>();
                mapObj->attributeIds.push_back(1);
                attributeMapping->registerMapping(1, QStringLiteral("contour"), contourStyleName);
                attributeMapping->verifyRequiredMappingRegistered();
                mapObj->attributeMapping = attributeMapping;
                mapObjects << mapObj;
                
                const auto newTiledData = std::make_shared<GeoTileObjectsProvider::Data>(
                    request.tileId,
                    request.zoom,
                    MapSurfaceType::Undefined,
                    mapObjects);

                // Publish new empty tile
                outData = newTiledData;
                addDataToCache(newTiledData);

                // Store weak reference to new tile and mark it as 'Loaded'
                tileEntry->dataIsPresent = true;
                tileEntry->dataWeakRef = newTiledData;
                tileEntry->setState(TileState::Loaded);
                
                /*
                outData.reset();

                // Store flag that there was no data and mark tile entry as 'Loaded'
                tileEntry->dataIsPresent = false;
                tileEntry->setState(TileState::Loaded);
                 */
            }
            result = true;
        };
        
    owner->_resourcesManager->obtainData(_request, _callback);
    
    if (!result)
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        outData.reset();
    }

    // Notify that tile has been loaded
    {
        QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
        tileEntry->loadedCondition.wakeAll();
    }
    
    return true;
}
