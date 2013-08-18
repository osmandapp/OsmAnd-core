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
#ifndef __OFFLINE_MAP_RASTER_TILE_PROVIDER_H_
#define __OFFLINE_MAP_RASTER_TILE_PROVIDER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QMutex>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd {

    class OfflineMapDataProvider;
    class OfflineMapRasterTileProvider_P;
    class OSMAND_CORE_API OfflineMapRasterTileProvider : public IMapBitmapTileProvider
    {
    private:
        const std::unique_ptr<OfflineMapRasterTileProvider_P> _d;
    protected:
    public:
        OfflineMapRasterTileProvider(const std::shared_ptr<OfflineMapDataProvider>& dataProvider, const float& displayDensity);
        virtual ~OfflineMapRasterTileProvider();

        const std::shared_ptr<OfflineMapDataProvider> dataProvider;

        const float displayDensity;
        const uint32_t tileSize;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTileImmediate(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<IMapTileProvider::Tile>& tile);
        virtual void obtainTileDeffered(const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback);
    };

}

#endif // __OFFLINE_MAP_RASTER_TILE_PROVIDER_H_
