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
    const auto x = heixelSizeN + halfHeixelSizeN + std::clamp(coordinates.x, 0.0f, 1.0f) * (1.0f - 3.0f * heixelSizeN);
    const auto y = heixelSizeN + halfHeixelSizeN + std::clamp(coordinates.y, 0.0f, 1.0f) * (1.0f - 3.0f * heixelSizeN);

    const auto row = std::max(0, static_cast<int>(std::round(y * static_cast<float>(size))));
    const auto col = std::max(0, static_cast<int>(std::round(x * static_cast<float>(size))));

    outValue = *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row * rowLength) + col);

    return true;
}

bool OsmAnd::IMapElevationDataProvider::Data::getClosestPoint(
    const float startElevationFactor, const float endElevationFactor,
    const PointF& startCoordinates, const float startElevation,
    const PointF& endCoordinates, const float endElevation,
    PointF& outCoordinates, float* outHeightInMeters /*= nullptr*/) const
{
    // NOTE: Must be in sync with how renderer computes UV coordinates for elevation data
    const auto heixelOffset = heixelSizeN + halfHeixelSizeN;
    const auto heixelScale = (1.0f - 3.0f * heixelSizeN);

    const auto startHeixelN = PointF(
        std::max(0.0f, heixelOffset + std::clamp(startCoordinates.x, 0.0f, 1.0f) * heixelScale),
        std::max(0.0f, heixelOffset + std::clamp(startCoordinates.y, 0.0f, 1.0f) * heixelScale));
    const auto endHeixelN = PointF(
        std::max(0.0f, heixelOffset + std::clamp(endCoordinates.x, 0.0f, 1.0f) * heixelScale),
        std::max(0.0f, heixelOffset + std::clamp(endCoordinates.y, 0.0f, 1.0f) * heixelScale));
    
    const auto startHeixel = PointI(static_cast<int>(std::round(startHeixelN.x * static_cast<float>(size))),
        static_cast<int>(std::round(startHeixelN.y * static_cast<float>(size))));
    const auto deltaX = endHeixelN.x - startHeixelN.x;
    const auto deltaY = endHeixelN.y - startHeixelN.y;
    const auto maxDelta = std::max(std::fabs(deltaX), std::fabs(deltaY));
    const auto heixelCount = static_cast<int>(std::ceil(maxDelta * static_cast<float>(size - 3)));
    const auto rateX = deltaX / maxDelta;
    const auto rateY = deltaY / maxDelta;
    const auto rateZ = (endElevation - startElevation) / static_cast<float>(heixelCount);
    const auto rateE = (endElevationFactor - startElevationFactor) / static_cast<float>(heixelCount);
    auto col = startHeixel.x;
    auto row = startHeixel.y;
    auto rayHeight = startElevation * startElevationFactor;
    for (int index = 0; index <= heixelCount; index++)
    {
        const auto height =
            *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row * rowLength) + col);
        if (height >= rayHeight)
        {
            outCoordinates = PointF((static_cast<float>(col) / static_cast<float>(size) - heixelOffset) / heixelScale,
                (static_cast<float>(row) / static_cast<float>(size) - heixelOffset) / heixelScale);
            if (outHeightInMeters)
                *outHeightInMeters = height;
            return true;
        }
        const auto nextIndex = static_cast<float>(index + 1);
        col = startHeixel.x + static_cast<int>(std::round(rateX * nextIndex));
        row = startHeixel.y + static_cast<int>(std::round(rateY * nextIndex));
        rayHeight = (startElevation + rateZ * nextIndex) * (startElevationFactor + rateE * nextIndex);
    }
    return false;
}
