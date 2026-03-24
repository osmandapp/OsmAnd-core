#include "MapRenderer3DObjectsResource.h"

#include "MapRendererResourcesManager.h"
#include "MapRendererTiledResourcesCollection.h"
#include "IMapTiledDataProvider.h"
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include "Map3DObjectsProvider.h"
#include "Utilities.h"
#include "MapRenderer.h"
#include <iostream>

using namespace OsmAnd;

MapRenderer3DObjectsResource::MapRenderer3DObjectsResource(
    MapRendererResourcesManager* const owner,
    const TiledEntriesCollection<MapRendererBaseTiledResource>& collection,
    const TileId tileId,
    const ZoomLevel zoom)
    : MapRendererBaseTiledResource(owner, MapRendererResourceType::Map3DObjects, collection, tileId, zoom)
{
}

MapRenderer3DObjectsResource::~MapRenderer3DObjectsResource()
{
    safeUnlink();
}

bool MapRenderer3DObjectsResource::getProvider(std::shared_ptr<IMapDataProvider>& provider) const
{
    const auto link_ = link.lock();
    if (!link_)
    {
        return false;
    }

    const auto collection = static_cast<MapRendererTiledResourcesCollection*>(&link_->collection);
    if (!resourcesManager->obtainProviderFor(static_cast<MapRendererBaseResourcesCollection*>(collection), provider))
    {
        return false;
    }

    return true;
}

bool MapRenderer3DObjectsResource::supportsObtainDataAsync() const
{
    std::shared_ptr<IMapDataProvider> provider;
    if (!getProvider(provider))
    {
        return false;
    }

    return provider->supportsNaturalObtainDataAsync();
}

bool MapRenderer3DObjectsResource::obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController)
{
    std::shared_ptr<IMapDataProvider> iProvider;
    if (!getProvider(iProvider))
    {
        return false;
    }

    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(iProvider);

    // Obtain tile from provider
    std::shared_ptr<IMapTiledDataProvider::Data> tile;
    IMapTiledDataProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    const auto mapState = resourcesManager->renderer->getMapState();
    const auto visibleArea = Utilities::roundBoundingBox31(mapState.visibleBBox31, mapState.zoomLevel);
    request.visibleArea31 = Utilities::getEnlargedVisibleArea(visibleArea);
    const auto currentTime = QDateTime::currentMSecsSinceEpoch();
    request.areaTime = currentTime;
    request.queryController = queryController;

    const bool requestSucceeded = provider->obtainTiledData(request, tile);
    if (!requestSucceeded)
        return false;
    dataAvailable = static_cast<bool>(tile);

    // Store data
    if (dataAvailable)
    {
        QWriteLocker scopedLocker(&_sourceDataLock);

        _sourceData = std::static_pointer_cast<Map3DObjectsTiledProvider::Data>(tile);
    }

    return true;
}

void MapRenderer3DObjectsResource::obtainDataAsync(ObtainDataAsyncCallback callback,
    const std::shared_ptr<const IQueryController>& queryController, const bool cacheOnly)
{
    std::shared_ptr<IMapDataProvider> iProvider;
    if (!getProvider(iProvider))
    {
        callback(false, false);
        return;
    }

    const auto provider = std::static_pointer_cast<IMapTiledDataProvider>(iProvider);

    IMapTiledDataProvider::Request request;
    request.tileId = tileId;
    request.zoom = zoom;
    const auto mapState = resourcesManager->renderer->getMapState();
    const auto visibleArea = Utilities::roundBoundingBox31(mapState.visibleBBox31, mapState.zoomLevel);
    request.visibleArea31 = Utilities::getEnlargedVisibleArea(visibleArea);
    const auto currentTime = QDateTime::currentMSecsSinceEpoch();
    request.areaTime = currentTime;
    request.cacheOnly = cacheOnly;
    request.queryController = queryController;
    provider->obtainDataAsync(request,
        [this, callback]
        (const IMapDataProvider* const provider,
            const bool requestSucceeded,
            const std::shared_ptr<IMapDataProvider::Data>& data,
            const std::shared_ptr<Metric>& metric)
        {
            const auto dataAvailable = static_cast<bool>(data);

            // Store data
            if (dataAvailable)
            {
                QWriteLocker scopedLocker(&_sourceDataLock);

                _sourceData = std::static_pointer_cast<Map3DObjectsTiledProvider::Data>(data);
            }

            callback(requestSucceeded, dataAvailable);
        });
}

bool MapRenderer3DObjectsResource::uploadToGPU()
{
    std::shared_ptr<Map3DObjectsTiledProvider::Data> sourceData;
    {
        QReadLocker scopedLocker(&_sourceDataLock);

        sourceData = _sourceData;
    }

    if (!sourceData)
        return true;

    bool ok = resourcesManager->uploadTiled3DBuildingsToGPU(
        sourceData->vertices, sourceData->indices, sourceData->parts, _meshInGPU);
    if (!ok)
        return false;

    QWriteLocker scopedLocker(&_sourceDataLock);

    _sourceData.reset();

    return true;
}

void MapRenderer3DObjectsResource::captureResourceInGPU(
    std::shared_ptr<const OsmAnd::GPUAPI::MeshInGPU>& meshInGPU) const
{
    REPEAT_UNTIL(resourceInGPULock.testAndSetAcquire(0, 1));
    meshInGPU = _meshInGPU;
    resourceInGPULock.storeRelease(0);
}

void MapRenderer3DObjectsResource::unloadFromGPU()
{
    REPEAT_UNTIL(resourceInGPULock.testAndSetAcquire(0, 1));
    _meshInGPU.reset();
    resourceInGPULock.storeRelease(0);
}

void MapRenderer3DObjectsResource::lostDataInGPU()
{
    if (_meshInGPU)
        _meshInGPU->lostRefInGPU();
    _meshInGPU.reset();
}

void MapRenderer3DObjectsResource::releaseData()
{
    QWriteLocker scopedLocker(&_sourceDataLock);

    _sourceData.reset();
}
