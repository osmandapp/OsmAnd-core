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
                , detailedZoom(InvalidZoomLevel)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            bool matches(const std::shared_ptr<MapRasterLayerCache>& cache, const QByteArray& key,
                const MapPrimitivesProvider::Request& request) const
            {
                const auto detail = request.detailedZoom == InvalidZoomLevel ? request.zoom : request.detailedZoom;
                return rasterCache.lock() == cache && rasterCacheKey == key
                    && (detailedZoom == detail || (!cache && request.detailedZoom == InvalidZoomLevel));
            }

            bool dataIsPresent;
            std::weak_ptr<MapPrimitivesProvider::Data> dataWeakRef;
            std::weak_ptr<MapRasterLayerCache> rasterCache;
            QByteArray rasterCacheKey;
            ZoomLevel detailedZoom;

            QReadWriteLock loadedConditionLock;
            QWaitCondition loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;
        mutable TiledEntriesCollection<TileEntry> _symbolTileReferences;

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
        mutable QMutex _rasterCacheMutex;
        std::shared_ptr<MapRasterLayerCache> _rasterCache;
        using RasterRequest = std::shared_ptr<const MapPrimitivesProvider::Request>;
        QMultiHash<QPair<int, quint64>, RasterRequest> _rasterHits;

        bool loadPrimitives(const MapPrimitivesProvider::Request& request,
            std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
            MapPrimitivesProvider_Metrics::Metric_obtainData* metric, bool allowSymbolsOnly);
    public:
        // Retain the visible tile's detail zoom and viewport for polygon queries after an image-only hit.
        struct RasterHitMetadata final : IMapDataProvider::RetainableCacheMetadata
        {
            RasterHitMetadata(const std::shared_ptr<MapPrimitivesProvider_P>& provider,
                const std::shared_ptr<MapRasterLayerCache>& cache, const MapPrimitivesProvider::Request& request);
            ~RasterHitMetadata() Q_DECL_OVERRIDE;
            std::weak_ptr<MapPrimitivesProvider_P> provider;
            RasterRequest request;
        };
        ~MapPrimitivesProvider_P();

        ImplementationInterface<MapPrimitivesProvider> owner;

        std::shared_ptr<MapRasterLayerCache> getRasterCache() const;
        void setRasterCache(const std::shared_ptr<MapRasterLayerCache>& cache);
        void clearRasterCache(const std::shared_ptr<MapRasterLayerCache>& cache);

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);
        bool obtainTiledPrimitives(
            const MapPrimitivesProvider::Request& request,
            std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
            MapPrimitivesProvider_Metrics::Metric_obtainData* const metric_);
        QList<std::shared_ptr<const OsmAnd::MapObject>> retreivePolygons(PointI point, ZoomLevel zoom);

    friend class OsmAnd::MapPrimitivesProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_P_H_)
