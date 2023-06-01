#ifndef _OSMAND_CORE_TRACKAREA_H_
#define _OSMAND_CORE_TRACKAREA_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Utilities.h>

namespace OsmAnd
{
    struct TrackArea
    {
        AreaI64 bbox;
        PointI64 last;

        inline TrackArea()
        {
            bbox = AreaI64::negative();
        }

        inline TrackArea(const TrackArea& that)
        {
            this->bbox = that.bbox;
            this->last = that.last;
        }

        inline TrackArea(const PointI& start)
        {
            bbox.topLeft = start;
            bbox.bottomRight = start;
            last = start;
        }

        inline void add(const PointI point)
        {
            if (bbox.left() == 1 && bbox.right() == -1)
            {
                bbox.topLeft = point;
                bbox.bottomRight = point;
                last = point;
            }
            else
            {
                const auto point31 = Utilities::normalizeCoordinates(last, ZoomLevel31);
                auto offset = point - point31;
                const auto intHalf = INT32_MAX / 2 + 1;
                if (offset.x >= intHalf)
                    offset.x = offset.x - INT32_MAX - 1;
                else if (offset.x < -intHalf)
                    offset.x = offset.x + INT32_MAX + 1;
                const auto nextPoint31 = Utilities::normalizeCoordinates(PointI64(point31) + offset, ZoomLevel31);
                Utilities::calculateShortestPath(last, point31, nextPoint31, bbox.topLeft, bbox.bottomRight);
                last += offset;
                bbox.enlargeToInclude(last);
            }
        }

        inline void update(const QVector<PointI>& path, const int index = 0)
        {
            const auto intHalf = INT32_MAX / 2 + 1;
            auto start = index;
            PointI point31;
            auto point64 = last;
            if (bbox.left() == 1 && bbox.right() == -1)
            {
                point31 = path[start++];
                point64 = point31;
                bbox.topLeft = point64;
                bbox.bottomRight = point64;
            }
            else
                point31 = Utilities::normalizeCoordinates(point64, ZoomLevel31);
            PointI nextPoint31;
            for (int i = start; i < path.size(); i++)
            {
                auto offset = path[i] - point31;
                if (offset.x >= intHalf)
                    offset.x = offset.x - INT32_MAX - 1;
                else if (offset.x < -intHalf)
                    offset.x = offset.x + INT32_MAX + 1;
                nextPoint31 = Utilities::normalizeCoordinates(PointI64(point31) + offset, ZoomLevel31);
                Utilities::calculateShortestPath(point64, point31, nextPoint31, bbox.topLeft, bbox.bottomRight);
                point64 += offset;
                bbox.enlargeToInclude(point64);
                point31 = nextPoint31;
            }
            last = point64;
        }

        inline AreaI normalized()
        {
            const int64_t intMin = INT32_MIN;
            const int64_t intMax = INT32_MAX;
            const int64_t intFull = bbox.left() == 1 && bbox.right() == -1 ? 0 : intMax + 1;
            const PointI64 shiftBack(bbox.topLeft.x < 0 ? 0 : intFull, bbox.topLeft.y < 0 ? 0 : intFull);
            const PointI64 topLeft(bbox.topLeft - shiftBack);
            const PointI64 bottomRight(bbox.bottomRight - shiftBack);
            const PointI topLeft31(
                static_cast<int32_t>(qBound(intMin, topLeft.x, intMax)),
                static_cast<int32_t>(qBound(intMin, topLeft.y, intMax)));
            const PointI bottomRight31(
                static_cast<int32_t>(qBound(intMin, bottomRight.x, intMax)),
                static_cast<int32_t>(qBound(intMin, bottomRight.y, intMax)));
            return AreaI(topLeft31, bottomRight31);
        }
    };
}

#endif // !defined(_OSMAND_CORE_TRACKAREA_H_)
