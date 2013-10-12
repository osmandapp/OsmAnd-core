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

#include <cstdint>
#include <memory>

#include <QDir>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>

namespace OsmAnd {

    class HeightmapTileProvider_P;
    class OSMAND_CORE_API HeightmapTileProvider : public IMapElevationDataProvider
    {
    private:
        const std::unique_ptr<HeightmapTileProvider_P> _d;
    protected:
    public:
        HeightmapTileProvider(const QDir& dataPath, const QString& indexFilepath = QString());
        virtual ~HeightmapTileProvider();

        void rebuildTileDbIndex();

        static const QString defaultIndexFilename;

        virtual uint32_t getTileSize() const;
        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<MapTile>& outTile);
    };

}

#endif // __HEIGHTMAP_TILE_PROVIDER_H_
