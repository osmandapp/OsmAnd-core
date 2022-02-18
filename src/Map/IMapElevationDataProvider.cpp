#include "IMapElevationDataProvider.h"

#include "MapRenderer.h"
#include "MapDataProviderHelpers.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider() = default;

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider() = default;

bool OsmAnd::IMapElevationDataProvider::obtainElevationData(
    const Request& request, std::shared_ptr<Data>& outElevationData, std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
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
    , heixelSizeN(1.0f / (float)size)
    , halfHeixelSizeN(0.5f * heixelSizeN)
{
}

OsmAnd::IMapElevationDataProvider::Data::~Data()
{
    delete[] pRawData;

    release();
}

bool OsmAnd::IMapElevationDataProvider::Data::getValue(const OsmAnd::PointF& coordinates, float& outValue) const
{
    // NOTE: Must be in sync with how renderer computes UV coordinates for elevation data
    const auto x = halfHeixelSizeN + std::clamp(coordinates.x, 0.0f, 1.0f) * (1.0f - 3.0f * heixelSizeN);
    const auto y = halfHeixelSizeN + std::clamp(coordinates.y, 0.0f, 1.0f) * (1.0f - 3.0f * heixelSizeN);

    const auto row = std::max(0, static_cast<int>(std::round(y * static_cast<float>(size))));
    const auto col = std::max(0, static_cast<int>(std::round(x * static_cast<float>(size))));

    outValue = *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row * rowLength) + col);

    return true;
}
