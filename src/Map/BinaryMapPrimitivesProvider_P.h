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
#include "BinaryMapPrimitivesProvider_Metrics.h"

namespace OsmAnd
{
    class BinaryMapPrimitivesTile;
    class BinaryMapPrimitivesTile_P;

    class BinaryMapPrimitivesProvider;
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

            std::weak_ptr<BinaryMapPrimitivesTile> _tile;

            QReadWriteLock _loadedConditionLock;
            QWaitCondition _loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        const std::shared_ptr<Primitiviser::Cache> _primitiviserCache;
    public:
        ~BinaryMapPrimitivesProvider_P();

        ImplementationInterface<BinaryMapPrimitivesProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

    friend class OsmAnd::BinaryMapPrimitivesProvider;
    friend class OsmAnd::BinaryMapPrimitivesTile_P;
    };

    class BinaryMapPrimitivesTile;
    class BinaryMapPrimitivesTile_P Q_DECL_FINAL
    {
    private:
    protected:
        BinaryMapPrimitivesTile_P(BinaryMapPrimitivesTile* owner);

        std::weak_ptr<BinaryMapPrimitivesProvider_P::TileEntry> _refEntry;

        void cleanup();
    public:
        virtual ~BinaryMapPrimitivesTile_P();

        ImplementationInterface<BinaryMapPrimitivesTile> owner;

    friend class OsmAnd::BinaryMapPrimitivesTile;
    friend class OsmAnd::BinaryMapPrimitivesProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_P_H_)
