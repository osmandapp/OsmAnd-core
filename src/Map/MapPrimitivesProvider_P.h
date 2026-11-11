#ifndef _OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_P_H_

#include "stdlib_common.h"
#include "ignore_warnings_on_external_includes.h"
#include <utility>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QHash>
#include <QAtomicInt>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QElapsedTimer>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "Link.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledDataProvider.h"
#include "TiledEntriesCollection.h"
#include "ObfMapSectionReader.h"
#include "MapPrimitiviser.h"
#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapTileDiskCache.h"

namespace OsmAnd
{
    class MapPrimitivesProvider_P Q_DECL_FINAL
    {
    protected:
        MapPrimitivesProvider_P(MapPrimitivesProvider* owner);

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Cancelled
        };
        struct TileEntry : TiledEntriesCollectionEntryWithState < TileEntry, TileState, TileState::Undefined >
        {
            TileEntry(const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
                , dataIsPresent(false)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            bool dataIsPresent;
            std::weak_ptr<MapPrimitivesProvider::Data> dataWeakRef;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        const std::shared_ptr<MapPrimitiviser::Cache> _primitiviserCache;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<TileEntry>& tileEntry,
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::weak_ptr<TileEntry> tileEntryWeakRef;
            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapRetainableCacheMetadata;
        };
    private:
        // disk cache: tiles of z <= 15 and their filtered symbols
        mutable QMutex _diskCacheMutex;
        std::shared_ptr<MapTileDiskCache> _diskCache;
        QString _diskCacheStyleKey;
        mutable uint64_t _diskCacheMapsSignature;
        mutable bool _diskCacheMapsSynced;
        mutable QElapsedTimer _diskCacheMapsCheck;
        mutable QHash< QString, std::shared_ptr<const ObfMapSectionInfo> > _diskCacheSections;
        mutable QAtomicInteger<quint64> _diskCacheLastEnvironment;
        uint64_t computeDiskCacheEnvironment() const;
        bool syncDiskCacheMaps(const std::shared_ptr<MapTileDiskCache>& diskCache) const;
        bool obtainCachedSymbols(
            const MapPrimitivesProvider::Request& request,
            std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives);
        void storeCachedSymbols(
            const MapPrimitivesProvider::Request& request,
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
            const QList< std::shared_ptr<const MapObject> >& mapObjects,
            const MapSurfaceType surfaceType);

        void collectPolygons(QList<std::shared_ptr<const OsmAnd::MapObject>> & polygons, 
                             const std::shared_ptr<const MapObject> & mapObj,
                             const MapPrimitiviser::PrimitiveType & type, const PointI & point);
    public:
        ~MapPrimitivesProvider_P();

        ImplementationInterface<MapPrimitivesProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);
        bool obtainTiledPrimitives(
            const MapPrimitivesProvider::Request& request,
            std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
            MapPrimitivesProvider_Metrics::Metric_obtainData* const metric_);
        QList<std::shared_ptr<const OsmAnd::MapObject>> retreivePolygons(PointI point, ZoomLevel zoom);

        void setDiskCache(const std::shared_ptr<MapTileDiskCache>& diskCache, const QString& styleKey);
        std::shared_ptr<MapTileDiskCache> getDiskCache(const ZoomLevel zoom, uint64_t& outEnvironment) const;

    friend class OsmAnd::MapPrimitivesProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_P_H_)
