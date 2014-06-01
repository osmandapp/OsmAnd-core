#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_

#include "stdlib_common.h"
#include <utility>

#include "QtExtensions.h"
#include <QHash>
#include <QAtomicInt>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledDataProvider.h"
#include "TiledEntriesCollection.h"
#include "SharedByZoomResourcesContainer.h"

namespace OsmAnd
{
    namespace Model
    {
        class MapObject;
    }
    class BinaryMapDataTile;
    class BinaryMapDataTile_P;
    class RasterizerContext;

    class BinaryMapDataProvider;
    class BinaryMapDataProvider_P
    {
    private:
    protected:
        BinaryMapDataProvider_P(BinaryMapDataProvider* owner);

        mutable SharedByZoomResourcesContainer<uint64_t, const Model::MapObject> _sharedMapObjects;

        enum class TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Released
        };
        struct TileEntry : TiledEntriesCollectionEntryWithState<TileEntry, TileState, TileState::Undefined>
        {
            TileEntry(const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TiledEntriesCollectionEntryWithState(collection, tileId, zoom)
            {
            }

            virtual ~TileEntry()
            {
                safeUnlink();
            }

            std::weak_ptr<const BinaryMapDataTile> _tile;

            QReadWriteLock _loadedConditionLock;
            QWaitCondition _loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        class Link : public std::enable_shared_from_this<Link>
        {
        private:
        protected:
        public:
            Link(BinaryMapDataProvider_P& provider_)
                : provider(provider_)
            {
            }

            virtual ~Link()
            {
            }

            BinaryMapDataProvider_P& provider;
        };
        const std::shared_ptr<Link> _link;
    public:
        ~BinaryMapDataProvider_P();

        ImplementationInterface<BinaryMapDataProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<const MapTiledData>& outTiledData,
            const IQueryController* const queryController);

    friend class OsmAnd::BinaryMapDataProvider;
    friend class OsmAnd::BinaryMapDataTile_P;
    };

    class BinaryMapDataTile;
    class BinaryMapDataTile_P
    {
    private:
    protected:
        BinaryMapDataTile_P(BinaryMapDataTile* owner);

        ImplementationInterface<BinaryMapDataTile> owner;

        std::weak_ptr<BinaryMapDataProvider_P::Link> _link;
        std::weak_ptr<BinaryMapDataProvider_P::TileEntry> _refEntry;

        QList< std::shared_ptr<const Model::MapObject> > _mapObjects;
        std::shared_ptr< const RasterizerContext > _rasterizerContext;

        void cleanup();
    public:
        virtual ~BinaryMapDataTile_P();

    friend class OsmAnd::BinaryMapDataTile;
    friend class OsmAnd::BinaryMapDataProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_P_H_)
