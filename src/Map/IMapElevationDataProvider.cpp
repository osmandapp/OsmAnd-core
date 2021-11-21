#include "IMapElevationDataProvider.h"

#include "MapRenderer.h"
#include "MapDataProviderHelpers.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}

bool OsmAnd::IMapElevationDataProvider::obtainElevationData(
    const Request& request,
    std::shared_ptr<Data>& outElevationData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outElevationData, pOutMetric);
}

OsmAnd::IMapElevationDataProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const size_t rowLength_,
    const uint32_t size_,
    const float* const pRawData_)
    : IMapTiledDataProvider::Data(tileId_, zoom_)
    , rowLength(rowLength_)
    , size(size_)
    , pRawData(pRawData_)
{
}

OsmAnd::IMapElevationDataProvider::Data::~Data()
{
    delete[] pRawData;

    release();
}
