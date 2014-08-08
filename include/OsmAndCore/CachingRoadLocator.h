#ifndef _OSMAND_CORE_CACHING_ROAD_LOCATOR_H_
#define _OSMAND_CORE_CACHING_ROAD_LOCATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IRoadLocator.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;
    namespace Model
    {
        class Road;
    }

    class CachingRoadLocator_P;
    class OSMAND_CORE_API CachingRoadLocator : public IRoadLocator
    {
        Q_DISABLE_COPY(CachingRoadLocator);
    private:
        PrivateImplementation<CachingRoadLocator_P> _p;
    protected:
    public:
        CachingRoadLocator(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~CachingRoadLocator();

        const std::shared_ptr<const IObfsCollection> obfsCollection;

        virtual std::shared_ptr<const Model::Road> findNearestRoad(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            int* const outNearestRoadPointIndex = nullptr,
            double* const outDistanceToNearestRoadPoint = nullptr) const;
        virtual QList< std::shared_ptr<const Model::Road> > findRoadsInArea(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel) const;

        void clearCache();
        void clearCacheConditional(const std::function<bool (const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock)> shouldRemoveFromCacheFunctor);
        void clearCacheInBBox(const AreaI bbox31, const bool checkAlsoIntersection);
        void clearCacheInTiles(const QSet<TileId> tiles, const ZoomLevel zoomLevel, const bool checkAlsoIntersection);
        void clearCacheNotInBBox(const AreaI bbox31, const bool checkAlsoIntersection);
        void clearCacheNotInTiles(const QSet<TileId> tiles, const ZoomLevel zoomLevel, const bool checkAlsoIntersection);
    };
}

#endif // !defined(_OSMAND_CORE_CACHING_ROAD_LOCATOR_H_)
