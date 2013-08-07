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
#ifndef __MAP_RENDERER_TILED_RESOURCES_H_
#define __MAP_RENDERER_TILED_RESOURCES_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <QMap>
#include <QReadWriteLock>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <MapTypes.h>
#include <IMapTileProvider.h>
#include <RenderAPI.h>
#include <MapRenderer.h>

namespace OsmAnd {

    class OSMAND_CORE_API MapRendererTiledResources
    {
    public:
        class OSMAND_CORE_API TileEntry
        {
        public:
            enum State
            {
                // Tile is not in any determined state (tile entry did not exist)
                Unknown = 0,

                // Tile is not available at all
                Unavailable,

                // Tile was requested and should arrive soon
                Requested,

                // Tile data is in main memory, but not yet uploaded into GPU
                Ready,

                // Tile data is already in GPU
                Uploaded,

                // Tile is unloaded, next state is death by deallocation
                Unloaded,
            };

        private:
        protected:
            TileEntry(const TileId& tileId, const ZoomLevel& zoom);

            volatile State _state;
            std::shared_ptr<IMapTileProvider::Tile> _sourceData;
            std::shared_ptr<RenderAPI::ResourceInGPU> _resourceInGPU;
        public:
            virtual ~TileEntry();

            const TileId tileId;
            const ZoomLevel zoom;

            const volatile State& state;
            QReadWriteLock stateLock;

            const std::shared_ptr<IMapTileProvider::Tile>& sourceData;
            const std::shared_ptr<RenderAPI::ResourceInGPU>& resourceInGPU;

        friend class OsmAnd::MapRendererTiledResources;
        friend class OsmAnd::MapRenderer;
        };

    private:
        QMutex _tilesCollectionMutex;
        std::array< QMap< TileId, std::shared_ptr<TileEntry> > , ZoomLevelsCount > _tilesCollection;
    protected:
        MapRendererTiledResources(const MapRenderer::TiledResourceType& type);
    public:
        virtual ~MapRendererTiledResources();

        const MapRenderer::TiledResourceType type;

        std::shared_ptr<TileEntry> obtainTileEntry(const TileId& tileId, const ZoomLevel& zoom, bool createEmptyIfUnexistent = false);
        void obtainTileEntries(QList< std::shared_ptr<TileEntry> >& outList, unsigned int limit = 0u, const TileEntry::State& withState = TileEntry::State::Unknown, const ZoomLevel& andZoom = ZoomLevel::InvalidZoom);
        void removeAllEntries();

    friend class OsmAnd::MapRenderer;
    };

}

#endif // __MAP_RENDERER_TILED_RESOURCES_H_
