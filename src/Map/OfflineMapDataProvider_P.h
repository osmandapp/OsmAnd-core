#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <utility>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QAtomicInt>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "TilesCollection.h"
#include "SharedByZoomResourcesContainer.h"

namespace OsmAnd {

    namespace Model {
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

        OfflineMapDataProvider* const owner;

        SharedByZoomResourcesContainer<uint64_t, const Model::MapObject> _sharedMapObjects;

        enum TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Released
        };
        struct TileEntry : TilesCollectionEntryWithState<TileEntry, TileState, TileState::Undefined>
        {
            TileEntry(const TilesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TilesCollectionEntryWithState(collection, tileId, zoom)
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
        TilesCollection<TileEntry> _tileReferences;

        class Link : public std::enable_shared_from_this<Link>
        {
        private:
        protected:
        public:
            Link(OfflineMapDataProvider_P& provider_)
                : provider(provider_)
            {}

            virtual ~Link()
            {}

            OfflineMapDataProvider_P& provider;
        };
        const std::shared_ptr<Link> _link;
    public:
        ~OfflineMapDataProvider_P();

        void obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile);

    friend class OsmAnd::OfflineMapDataProvider;
    friend class OsmAnd::OfflineMapDataTile_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_)
