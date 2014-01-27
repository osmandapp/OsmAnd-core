/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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
#ifndef _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_H_
#define _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd {

    class OfflineMapDataProvider;
    class OfflineMapRasterTileProvider_GPU_P;
    class OSMAND_CORE_API OfflineMapRasterTileProvider_GPU : public IMapBitmapTileProvider
    {
        Q_DISABLE_COPY(OfflineMapRasterTileProvider_GPU);
    private:
        const std::unique_ptr<OfflineMapRasterTileProvider_GPU_P> _d;
    protected:
    public:
        OfflineMapRasterTileProvider_GPU(const std::shared_ptr<OfflineMapDataProvider>& dataProvider, const uint32_t outputTileSize = 256, const float density = 1.0f);
        virtual ~OfflineMapRasterTileProvider_GPU();

        const std::shared_ptr<OfflineMapDataProvider> dataProvider;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile);
    };

}

#endif // _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_H_
