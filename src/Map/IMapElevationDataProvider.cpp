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
    float xCoord;
    float yCoord;
    const auto xOffset = std::max(0.0f, std::modf(x - 0.5f, &xCoord));
    const auto yOffset = std::max(0.0f, std::modf(y - 0.5f, &yCoord));

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
    const auto tSize = static_cast<float>(size);
    const auto heixelOffset = (heixelSizeN + halfHeixelSizeN) * tSize;
    const auto heixelScale = (1.0f - 3.0f * heixelSizeN) * tSize;

    const auto x = heixelOffset + std::clamp(coordinates.x, 0.0f, 1.0f) * heixelScale;
    const auto y = heixelOffset + std::clamp(coordinates.y, 0.0f, 1.0f) * heixelScale;

    outValue = getInterpolatedValue(x, y);

    return true;
}

bool OsmAnd::IMapElevationDataProvider::Data::getClosestPoint(
    const float startElevationFactor, const float endElevationFactor,
    const PointF& startCoordinates, const float startElevation,
    const PointF& endCoordinates, const float endElevation,
    const float scaleFactor, PointF& outCoordinates, float* outHeightInMeters /*= nullptr*/) const
{
    // NOTE: Must be in sync with how renderer computes UV coordinates for elevation data
    const auto tSize = static_cast<float>(size);
    const auto heixelOffset = (heixelSizeN + halfHeixelSizeN) * tSize;
    const auto heixelScale = (1.0f - 3.0f * heixelSizeN) * tSize;

    const auto startHeixel = PointF(
        heixelOffset + std::clamp(startCoordinates.x, 0.0f, 1.0f) * heixelScale,
        heixelOffset + std::clamp(startCoordinates.y, 0.0f, 1.0f) * heixelScale);
    const auto endHeixel = PointF(
        heixelOffset + std::clamp(endCoordinates.x, 0.0f, 1.0f) * heixelScale,
        heixelOffset + std::clamp(endCoordinates.y, 0.0f, 1.0f) * heixelScale);
    
    auto stepCount = std::max(
        std::fabs(std::floor(endHeixel.x) - std::floor(startHeixel.x)),
        std::fabs(std::floor(endHeixel.y) - std::floor(startHeixel.y)));
    stepCount = std::max(1.0f, stepCount * scaleFactor * 2.0f);
    const auto rateX = (endHeixel.x - startHeixel.x) / stepCount;
    const auto rateY = (endHeixel.y - startHeixel.y) / stepCount;
    const auto rateZ = (endElevation - startElevation) / stepCount;
    const auto rateE = (endElevationFactor - startElevationFactor) / stepCount;
    auto coordX = startHeixel.x;
    auto coordY = startHeixel.y;
    auto rayHeight = startElevation * startElevationFactor;
    auto prevRayHeight = rayHeight;
    const auto count = static_cast<int>(std::round(stepCount));
    float prevHeight;
    for (int index = 0; index <= count; index++)
    {
        auto height = getInterpolatedValue(coordX, coordY);
        if (height >= rayHeight)
        {
            if (index > 0)
            {
                const auto prevIndex = static_cast<float>(index - 1);
                const auto prevCoordX = startHeixel.x + rateX * prevIndex;
                const auto prevCoordY = startHeixel.y + rateY * prevIndex;
                const auto prevDifference = prevRayHeight - prevHeight;
                auto distanceFactor = prevDifference / (height - rayHeight + prevDifference);
                distanceFactor = distanceFactor >= 0.0f && distanceFactor < 1.0f ? distanceFactor : 1.0f;
                coordX = prevCoordX + (coordX - prevCoordX) * distanceFactor;
                coordY = prevCoordY + (coordY - prevCoordY) * distanceFactor;
            }
            const auto tileHexelCount = tSize - 3.0f;
            const auto rayVector = endCoordinates - startCoordinates;
            const auto length = rayVector.norm();
            const auto distance =
                PointF((coordX - startHeixel.x) / tileHexelCount, (coordY - startHeixel.y) / tileHexelCount).norm();
            const auto factor = std::min(length, distance) / length;
            outCoordinates = startCoordinates + rayVector * factor;
            if (outHeightInMeters)
                *outHeightInMeters = height;
            return true;
        }
        const auto nextIndex = static_cast<float>(index + 1);
        coordX = startHeixel.x + rateX * nextIndex;
        coordY = startHeixel.y + rateY * nextIndex;
        prevRayHeight = rayHeight;
        rayHeight = (startElevation + rateZ * nextIndex) * (startElevationFactor + rateE * nextIndex);
        prevHeight = height;
    }
    return false;
}
