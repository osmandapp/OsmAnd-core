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
#ifndef __HILLSHADE_TILE_PROVIDER_H_
#define __HILLSHADE_TILE_PROVIDER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QString>
#include <QDir>
#include <QSqlDatabase>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

class SkBitmap;

namespace OsmAnd {

    class OSMAND_CORE_API HillshadeTileProvider : public IMapBitmapTileProvider
    {
    private:
    protected:
        class OSMAND_CORE_API Tile : public IMapBitmapTileProvider::Tile
        {
        private:
            std::unique_ptr<SkBitmap> _skBitmap;
        protected:
        public:
            Tile(SkBitmap* bitmap);
            virtual ~Tile();
        };

        QString _indexFilePath;
    public:
        HillshadeTileProvider(const QDir& storagePath);
        virtual ~HillshadeTileProvider();

        const QDir storagePath;
        
        void setIndexFilePath(const QString& indexFilePath);
        const QString& indexFilePath;

        void rebuildIndex();

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTileImmediate(const TileId& tileId, uint32_t zoom, std::shared_ptr<IMapTileProvider::Tile>& tile);
        virtual void obtainTileDeffered(const TileId& tileId, uint32_t zoom, TileReadyCallback readyCallback);
    };

}

#endif // __HILLSHADE_TILE_PROVIDER_H_