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

inline float OsmAnd::IMapElevationDataProvider::Data::getInterpolatedValue(const float x, const float y) const
{
    const auto tSize = static_cast<float>(size);
    float xCoord;
    float yCoord;
    const auto xOffset = std::max(0.0f, std::modf(x * tSize, &xCoord));
    const auto yOffset = std::max(0.0f, std::modf(y * tSize, &yCoord));

    auto row = std::max(0, static_cast<int>(yCoord));
    auto col = std::max(0, static_cast<int>(xCoord));

    row *= rowLength;
    const auto tlHeixel =
        *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row) + col++);
    const auto trHeixel =
        *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row) + col);
    row += rowLength;
    const auto brHeixel =
        *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row) + col--);
    const auto blHeixel =
        *(reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pRawData) + row) + col);

    const auto avtPixel = glm::mix(tlHeixel, trHeixel, xOffset);
    const auto avbPixel = glm::mix(blHeixel, brHeixel, xOffset);

    return glm::mix(avtPixel, avbPixel, yOffset);
}

bool OsmAnd::IMapElevationDataProvider::Data::getValue(const OsmAnd::PointF& coordinates, float& outValue) const
{
    // NOTE: Must be in sync with how renderer computes UV coordinates for elevation data
    const auto x = heixelSizeN + halfHeixelSizeN + std::clamp(coordinates.x, 0.0f, 1.0f) * (1.0f - 3.0f * heixelSizeN);
    const auto y = heixelSizeN + halfHeixelSizeN + std::clamp(coordinates.y, 0.0f, 1.0f) * (1.0f - 3.0f * heixelSizeN);

    outValue = getInterpolatedValue(x, y);

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
    
    const auto tSize = static_cast<float>(size);
    const auto startHeixel = PointI(static_cast<int>(std::round(startHeixelN.x * tSize)),
        static_cast<int>(std::round(startHeixelN.y * tSize)));
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
            outCoordinates = PointF((static_cast<float>(col) / tSize - heixelOffset) / heixelScale,
                (static_cast<float>(row) / tSize - heixelOffset) / heixelScale);
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
