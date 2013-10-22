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

#ifndef __OFFLINE_MAP_DATA_PROVIDER_P_H_
#define __OFFLINE_MAP_DATA_PROVIDER_P_H_

#include <cstdint>
#include <memory>

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

        QAtomicInt _basemapPresenceChecked;
        QMutex _basemapPresenceCheckMutex;
        volatile bool _isBasemapAvailable;

        struct DataCacheLevel
        {
            QReadWriteLock _mapObjectsMutex;
            QHash< uint64_t, std::weak_ptr< const Model::MapObject > > _mapObjects;
        };
        std::array< DataCacheLevel, ZoomLevelsCount> _dataCache;

        enum TileState
        {
            Undefined = -1,

            Loading,
            Loaded,
            Released
        };
        struct TileEntry : TilesCollectionEntryWithState<TileEntry, TileState, TileState::Undefined>
        {
            TileEntry(TilesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom)
                : TilesCollectionEntryWithState(collection, tileId, zoom)
            {}

            virtual ~TileEntry()
            {}

            std::weak_ptr< const OfflineMapDataTile > _tile;
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

        static uint64_t makeInternalId(const std::shared_ptr<const Model::MapObject>& mapObject);
    public:
        ~OfflineMapDataProvider_P();

        bool isBasemapAvailable();
        void obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile);

    friend class OsmAnd::OfflineMapDataProvider;
    friend class OsmAnd::OfflineMapDataTile_P;
    };

} // namespace OsmAnd

#endif // __OFFLINE_MAP_DATA_PROVIDER_P_H_
