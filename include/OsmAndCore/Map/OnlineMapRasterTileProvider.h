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
#ifndef __ONLINE_MAP_RASTER_TILE_PROVIDER_H_
#define __ONLINE_MAP_RASTER_TILE_PROVIDER_H_

#include <cstdint>
#include <memory>

#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd {

    class OnlineMapRasterTileProvider_P;
    class OSMAND_CORE_API OnlineMapRasterTileProvider : public IMapBitmapTileProvider
    {
    private:
        const std::unique_ptr<OnlineMapRasterTileProvider_P> _d;
    protected:
    public:
        OnlineMapRasterTileProvider(const QString& id, const QString& urlPattern,
            const ZoomLevel minZoom = MinZoomLevel, const ZoomLevel maxZoom = MaxZoomLevel,
            const uint32_t maxConcurrentDownloads = 1, const uint32_t providerTileSize = 256,
            const MapBitmapTile::AlphaChannelData& alphaChannelData = MapBitmapTile::AlphaChannelData::Undefined);
        virtual ~OnlineMapRasterTileProvider();

        const QString id;
        const QString urlPattern;
        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;
        const uint32_t maxConcurrentDownloads;
        const uint32_t providerTileSize;
        const MapBitmapTile::AlphaChannelData alphaChannelData;

        void setLocalCachePath(const QDir& localCachePath);
        const QDir& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<MapTile>& outTile);
        
        static std::shared_ptr<OsmAnd::IMapBitmapTileProvider> createMapnikProvider();
        static std::shared_ptr<OsmAnd::IMapBitmapTileProvider> createCycleMapProvider();
    };

}

#endif // __ONLINE_MAP_RASTER_TILE_PROVIDER_H_