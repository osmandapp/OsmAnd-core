#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_

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
#include "TiledEntriesCollection.h"
#include "SharedByZoomResourcesContainer.h"

namespace OsmAnd
{
    namespace Model
    {
        class MapObject;
    }
    class OfflineMapDataTile;
    class OfflineMapDataTile_P;

    class OfflineMapDataProvider;
    class OfflineMapDataProvider_P
    {
    private:
    protected:
        OfflineMapDataProvider_P(OfflineMapDataProvider* owner);

        ImplementationInterface<OfflineMapDataProvider> owner;

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

            std::weak_ptr< const OfflineMapDataTile > _tile;

            QReadWriteLock _loadedConditionLock;
            QWaitCondition _loadedCondition;
        };
        mutable TiledEntriesCollection<TileEntry> _tileReferences;

        class Link : public std::enable_shared_from_this<Link>
        {
        private:
        protected:
        public:
            Link(OfflineMapDataProvider_P& provider_)
                : provider(provider_)
            {
            }

            virtual ~Link()
            {
            }

            OfflineMapDataProvider_P& provider;
        };
        const std::shared_ptr<Link> _link;
    public:
        ~OfflineMapDataProvider_P();

        void obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile) const;

    friend class OsmAnd::OfflineMapDataProvider;
    friend class OsmAnd::OfflineMapDataTile_P;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_)
