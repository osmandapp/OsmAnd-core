#ifndef _OSMAND_CORE_CACHING_ROAD_LOCATOR_P_H_
#define _OSMAND_CORE_CACHING_ROAD_LOCATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QHash>
#include <QMutex>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "ObfRoutingSectionReader.h"

namespace OsmAnd
{
    class IObfsCollection;
    class Road;

    class CachingRoadLocator;
    class CachingRoadLocator_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CachingRoadLocator_P)
    private:
    protected:
        CachingRoadLocator_P(CachingRoadLocator* const owner);

        class Cache : public ObfRoutingSectionReader::DataBlocksCache
        {
        private:
        protected:
        public:
            Cache();
            virtual ~Cache();
        };
        mutable Cache _cache;

        mutable QMutex _referencedDataBlocksMapMutex;
        mutable QHash<
            const ObfRoutingSectionReader::DataBlock*,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> > > _referencedDataBlocksMap;
    public:
        ~CachingRoadLocator_P();

        ImplementationInterface<CachingRoadLocator> owner;

        std::shared_ptr<const Road> findNearestRoad(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter,
            int* const outNearestRoadPointIndex,
            double* const outDistanceToNearestRoadPoint) const;
        QVector<std::pair<std::shared_ptr<const Road>, double>> findNearestRoads(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries) const;
        QList<std::shared_ptr<const Road>> findRoadsInArea(
            const PointI position31,
            const double radiusInMeters,
            const RoutingDataLevel dataLevel,
            const ObfRoutingSectionReader::VisitorFunction filter) const;

        void clearCache();
        void clearCacheConditional(
            const std::function<bool (const std::shared_ptr<const ObfRoutingSectionReader::DataBlock>& dataBlock)> shouldRemoveFromCacheFunctor);
        void clearCacheInBBox(const AreaI bbox31, const bool checkAlsoIntersection);
        void clearCacheInTiles(const QSet<TileId>& tiles, const ZoomLevel zoomLevel, const bool checkAlsoIntersection);
        void clearCacheNotInBBox(const AreaI bbox31, const bool checkAlsoIntersection);
        void clearCacheNotInTiles(const QSet<TileId>& tiles, const ZoomLevel zoomLevel, const bool checkAlsoIntersection);

    friend class OsmAnd::CachingRoadLocator;
    };
}

#endif // !defined(_OSMAND_CORE_CACHING_ROAD_LOCATOR_P_H_)
