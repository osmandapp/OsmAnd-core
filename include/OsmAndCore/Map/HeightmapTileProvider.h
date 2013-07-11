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
#ifndef __HEIGHTMAP_TILE_PROVIDER_H_
#define __HEIGHTMAP_TILE_PROVIDER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>
#include <OsmAndCore/TileDB.h>

namespace OsmAnd {

    class OSMAND_CORE_API HeightmapTileProvider : public IMapElevationDataProvider
    {
    public:
    private:
    protected:
        class OSMAND_CORE_API Tile : public IMapElevationDataProvider::Tile
        {
        private:
        protected:
        public:
            Tile(const float* data, size_t rowLength, uint32_t width, uint32_t height);
            virtual ~Tile();
        };

        TileDB _tileDb;

        QMutex _processingMutex;
        QMutex _requestsMutex;
        std::array< QSet< TileId >, 32 > _requestedTileIds;
    public:
        HeightmapTileProvider(const QDir& dataPath, const QString& indexFilepath = QString());
        virtual ~HeightmapTileProvider();

        const TileDB& tileDb;
        void rebuildTileDbIndex();
        static const QString defaultIndexFilename;

        virtual uint32_t getTileSize() const;

        virtual bool obtainTileImmediate(const TileId& tileId, uint32_t zoom, std::shared_ptr<IMapTileProvider::Tile>& tile);
        virtual void obtainTileDeffered(const TileId& tileId, uint32_t zoom, TileReadyCallback readyCallback);
    };

}

#endif // __HEIGHTMAP_TILE_PROVIDER_H_
