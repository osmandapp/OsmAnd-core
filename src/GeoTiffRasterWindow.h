#ifndef _OSMAND_CORE_GEOTIFF_RASTER_WINDOW_H_
#define _OSMAND_CORE_GEOTIFF_RASTER_WINDOW_H_

#include <cmath>
#include <cstdint>

#include <QtGlobal>

namespace OsmAnd
{
    namespace GeoTiffRasterWindow
    {
        struct ClippedAxis
        {
            int32_t dataOffset = 0;
            int32_t dataSize = 0;
            int32_t tileOffset = 0;
            int32_t tileLength = 0;
            double sourceStart = 0.0;
            double sourceEnd = 0.0;
        };

        inline bool clipAxis(
            const double sourceStart,
            const double sourceEnd,
            const int32_t sourceLimit,
            const int32_t tileLimit,
            ClippedAxis& out)
        {
            out = ClippedAxis();
            if (sourceLimit <= 0 || tileLimit <= 0 ||
                !std::isfinite(sourceStart) || !std::isfinite(sourceEnd))
                return false;

            const auto sourceSpan = sourceEnd - sourceStart;
            if (!std::isfinite(sourceSpan) || sourceSpan <= 0.0)
                return false;

            const auto sourceResolution = sourceSpan / static_cast<double>(tileLimit);
            if (!std::isfinite(sourceResolution) || sourceResolution <= 0.0)
                return false;

            const auto clampToTile = [tileLimit](const double value) -> int32_t
            {
                if (!std::isfinite(value) || value >= static_cast<double>(tileLimit))
                    return tileLimit;
                if (value <= 0.0)
                    return 0;
                return static_cast<int32_t>(value);
            };

            int32_t destStart = 0;
            int32_t destEnd = tileLimit;
            if (sourceStart < 0.0)
                destStart = clampToTile(std::ceil(-sourceStart / sourceResolution));
            if (sourceEnd > static_cast<double>(sourceLimit))
                destEnd = clampToTile(std::floor((static_cast<double>(sourceLimit) - sourceStart) /
                    sourceResolution));

            destStart = qBound<int32_t>(0, destStart, tileLimit);
            destEnd = qBound<int32_t>(destStart, destEnd, tileLimit);
            out.tileOffset = destStart;
            out.tileLength = destEnd - destStart;
            if (out.tileLength <= 0)
                return false;

            out.sourceStart = sourceStart + static_cast<double>(destStart) * sourceResolution;
            out.sourceEnd = sourceStart + static_cast<double>(destEnd) * sourceResolution;
            if (!std::isfinite(out.sourceStart) || !std::isfinite(out.sourceEnd) ||
                out.sourceEnd <= out.sourceStart)
                return false;

            out.dataOffset = qBound<int32_t>(
                0, static_cast<int32_t>(std::floor(out.sourceStart)), sourceLimit);
            const auto dataEnd = qBound<int32_t>(
                0, static_cast<int32_t>(std::ceil(out.sourceEnd)), sourceLimit);
            out.dataSize = dataEnd - out.dataOffset;
            return out.dataSize > 0;
        }
    }
}

#endif // !defined(_OSMAND_CORE_GEOTIFF_RASTER_WINDOW_H_)
