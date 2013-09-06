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
#ifndef __TILES_COLLECTION_H_
#define __TILES_COLLECTION_H_

#include <cstdint>
#include <memory>
#include <array>
#include <functional>

#include <QMap>
#include <QMutex>
#include <QReadWriteLock>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    template<class ENTRY> class TilesCollection
    {
    private:
        std::array< QMap< TileId, std::shared_ptr<ENTRY> >, ZoomLevelsCount > _zoomLevels;
    protected:
        QMutex _tilesCollectionMutex;
    public:
        TilesCollection()
            : _tilesCollectionMutex(QMutex::Recursive)
        {
        }
        virtual ~TilesCollection()
        {
        }

        virtual bool obtainTileEntry(std::shared_ptr<ENTRY>& outEntry, const TileId& tileId, const ZoomLevel& zoom, bool createEmptyIfUnexistent = false)
        {
            QMutexLocker scopedLock(&_tilesCollectionMutex);

            auto& zoomLevel = _zoomLevels[zoom];
            auto itEntry = zoomLevel.find(tileId);
            if(itEntry != zoomLevel.end())
            {
                outEntry = *itEntry;
                return true;
            }

            if(!createEmptyIfUnexistent)
                return false;

            auto newEntry = new ENTRY(tileId, zoom);
            outEntry.reset(newEntry);
            itEntry = zoomLevel.insert(tileId, outEntry);
            return true;
        }
        virtual void obtainTileEntries(QList< std::shared_ptr<ENTRY> >* outList, std::function<bool (const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr)
        {
            QMutexLocker scopedLock(&_tilesCollectionMutex);

            bool doCancel = false;
            for(auto itZoomLevel = _zoomLevels.begin(); itZoomLevel != _zoomLevels.end(); ++itZoomLevel)
            {
                const auto& zoomLevel = *itZoomLevel;

                for(auto itEntryPair = zoomLevel.begin(); itEntryPair != zoomLevel.end(); ++itEntryPair)
                {
                    const auto& entry = itEntryPair.value();
                    
                    if(!filter || (filter && filter(entry, doCancel)))
                    {
                        if(outList)
                            outList->push_back(entry);
                    }

                    if(doCancel)
                        return;
                }
            }
        }
        virtual void removeAllEntries()
        {
            QMutexLocker scopedLock(&_tilesCollectionMutex);

            for(int zoom = ZoomLevel::MinZoomLevel; zoom != ZoomLevel::MaxZoomLevel; zoom++)
                _zoomLevels[zoom].clear();
        }
        virtual void removeEntry(const std::shared_ptr<ENTRY>& entry)
        {
            QMutexLocker scopedLock(&_tilesCollectionMutex);

            _zoomLevels[entry->zoom].remove(entry->tileId);
        }
        virtual void removeEntry(const TileId& tileId, const ZoomLevel& zoom)
        {
            QMutexLocker scopedLock(&_tilesCollectionMutex);

            _zoomLevels[zoom].remove(tileId);
        }
    };

    class OSMAND_CORE_API TilesCollectionEntry
    {
    private:
    protected:
        TilesCollectionEntry(const TileId& tileId, const ZoomLevel& zoom);
    public:
        virtual ~TilesCollectionEntry();

        const TileId tileId;
        const ZoomLevel zoom;
    };

    template<typename STATE_ENUM, STATE_ENUM UNDEFINED_STATE_VALUE> class TilesCollectionEntryWithState : public TilesCollectionEntry
    {
    private:
    protected:
    public:
        TilesCollectionEntryWithState(const TileId& tileId, const ZoomLevel& zoom, const STATE_ENUM& state = UNDEFINED_STATE_VALUE)
            : TilesCollectionEntry(tileId, zoom)
            , state(state)
        {
        }
        virtual ~TilesCollectionEntryWithState()
        {
        }

        volatile STATE_ENUM state;
        QReadWriteLock stateLock;
    };
}

#endif // __TILES_COLLECTION_H_
