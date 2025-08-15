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
            Loaded
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
        void collectPolygons(QList<std::shared_ptr<const OsmAnd::MapObject>> & polygons, 
                             const std::shared_ptr<const MapObject> & mapObj,
                             const MapObjectType & type, const PointI & point);
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

    friend class OsmAnd::MapPrimitivesProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_P_H_)
