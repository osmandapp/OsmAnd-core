#include "Map3DObjectsProvider.h"

#include "Map3DObjectsProvider_P.h"
#include "MapDataProviderHelpers.h"

using namespace OsmAnd;

Map3DObjectsTiledProvider::Data::Data(const TileId tileId, const ZoomLevel zoom, QVector<Building3D>&& buildings)
    : IMapTiledDataProvider::Data(tileId, zoom)
    , buildings3D(qMove(buildings))
{
}

Map3DObjectsTiledProvider::Data::~Data()
{
}

Map3DObjectsTiledProvider::Map3DObjectsTiledProvider(
    const std::shared_ptr<MapPrimitivesProvider>& tiledProvider,
    const std::shared_ptr<MapPresentationEnvironment>& environment)
    : _p(new Map3DObjectsTiledProvider_P(this, tiledProvider, environment))
{
}

Map3DObjectsTiledProvider::~Map3DObjectsTiledProvider()
{
}

ZoomLevel Map3DObjectsTiledProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

ZoomLevel Map3DObjectsTiledProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

bool Map3DObjectsTiledProvider::obtainTiledData(
    const IMapTiledDataProvider::Request& request,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    return _p->obtainTiledData(request, outTiledData, pOutMetric);
}

bool Map3DObjectsTiledProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool Map3DObjectsTiledProvider::supportsNaturalObtainData() const
{
    return _p->supportsNaturalObtainData();
}

bool Map3DObjectsTiledProvider::supportsNaturalObtainDataAsync() const
{
    return _p->supportsNaturalObtainDataAsync();
}

void Map3DObjectsTiledProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const ObtainDataAsyncCallback callback,
    const bool collectMetric)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

