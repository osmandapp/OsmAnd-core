#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_P_H_

#include "stdlib_common.h"
#include <utility>

#include "QtExtensions.h"
#include <QHash>
#include <QAtomicInt>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "Link.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledDataProvider.h"
#include "TiledEntriesCollection.h"
#include "ObfMapSectionReader.h"
#include "Primitiviser.h"
#include "BinaryMapPrimitivesProvider.h"
#include "BinaryMapPrimitivesProvider_Metrics.h"

namespace OsmAnd
{
    class BinaryMapPrimitivesProvider_P Q_DECL_FINAL
    {
    private:
    protected:
        BinaryMapPrimitivesProvider_P(BinaryMapPrimitivesProvider* owner);

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Released
        };
        struct TileEntry : TiledEntriesCollectionEntryWithState < TileEntry, TileState, TileState::Undefined >
        {
            TileEntry(const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            std::weak_ptr<BinaryMapPrimitivesProvider::Data> _tile;

            QReadWriteLock _loadedConditionLock;
            QWaitCondition _loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        const std::shared_ptr<Primitiviser::Cache> _primitiviserCache;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<TileEntry>& tileEntry,
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::weak_ptr<TileEntry> tileEntryWeakRef;
            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapRetainableCacheMetadata;
        };
    public:
        ~BinaryMapPrimitivesProvider_P();

        ImplementationInterface<BinaryMapPrimitivesProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapPrimitivesProvider::Data>& outTiledData,
            BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

    friend class OsmAnd::BinaryMapPrimitivesProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_P_H_)
