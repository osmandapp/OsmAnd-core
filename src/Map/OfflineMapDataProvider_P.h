/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QAtomicInt>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <TilesCollection.h>

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

        struct MapObjectsCacheLevel
        {
            mutable QReadWriteLock _lock;
            QHash< uint64_t, std::weak_ptr< const Model::MapObject > > _cache;
        };
        std::array< MapObjectsCacheLevel, ZoomLevelsCount> _mapObjectsCache;

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
            {}

            virtual ~TileEntry()
            {}

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

#endif // _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_P_H_
