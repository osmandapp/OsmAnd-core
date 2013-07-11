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
#ifndef __MAP_DATA_CACHE_H_
#define __MAP_DATA_CACHE_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <QList>
#include <QMap>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/Data/Model/MapObject.h>
#include <OsmAndCore/Data/ObfReader.h>

namespace OsmAnd {

    class OSMAND_CORE_API MapDataCache
    {
    private:
        MapDataCache(const MapDataCache& that);
    protected:
        QList< std::shared_ptr<OsmAnd::ObfReader> > _sources;

        const size_t _memoryLimit;
        QMutex _sourcesMutex;
        QMutex _cacheMutex;
        uint64_t _cachedObjects;
        size_t _approxConsumedMemory;

        struct CachedTile
        {
            size_t _approxConsumedMemory;
            QList< std::shared_ptr<OsmAnd::Model::MapObject> > _cachedObjects;
        };

        struct CachedZoomLevel
        {
            QMap< TileId, std::shared_ptr<CachedTile> > _cachedTiles;

            void obtainObjects(QList< std::shared_ptr<OsmAnd::Model::MapObject> >& resultOut, const AreaI& area31, uint32_t zoom, IQueryController* controller) const;
        };
        std::array< CachedZoomLevel, 32 > _zoomLevels;

        enum {
            CachePurgeZoomLevelThreshold = 2,
        };
    public:
        MapDataCache(const size_t& memoryLimit = std::numeric_limits<size_t>::max());
        virtual ~MapDataCache();

        void addSource(const std::shared_ptr<OsmAnd::ObfReader>& newSource);
        void removeSource(const std::shared_ptr<OsmAnd::ObfReader>& source);
        const QList< std::shared_ptr<OsmAnd::ObfReader> >& sources;

        const size_t &memoryLimit;
        const uint64_t& cachedObjects;
        const size_t &approxConsumedMemory;
        
        void purgeAllCache();
        void purgeCache(const AreaI& retainArea31, uint32_t retainZoom);
        bool isCached(const AreaI& area31, uint32_t zoom, IQueryController* controller = nullptr);
        void cacheObjects(const AreaI& area31, uint32_t zoom, IQueryController* controller = nullptr);

        void obtainObjects(QList< std::shared_ptr<OsmAnd::Model::MapObject> >& resultOut, const AreaI& area31, uint32_t zoom, IQueryController* controller = nullptr);
    };

}

#endif // __MAP_DATA_CACHE_H_