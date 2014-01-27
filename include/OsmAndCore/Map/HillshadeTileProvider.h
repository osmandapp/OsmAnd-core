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
#ifndef _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_
#define _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd {

    class HillshadeTileProvider_P;
    class OSMAND_CORE_API HillshadeTileProvider : public IMapBitmapTileProvider
    {
        Q_DISABLE_COPY(HillshadeTileProvider);
    private:
        const std::unique_ptr<HillshadeTileProvider> _d;
    protected:
    public:
        HillshadeTileProvider(const QDir& storagePath);
        virtual ~HillshadeTileProvider();

        const QDir storagePath;
        
        void setIndexFilePath(const QString& indexFilePath);
        const QString& indexFilePath;

        void rebuildIndex();

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile);
    };

}

#endif // _OSMAND_CORE_HILLSHADE_TILE_PROVIDER_H_