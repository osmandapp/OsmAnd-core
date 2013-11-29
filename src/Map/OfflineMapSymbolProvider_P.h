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
#ifndef _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QReadWriteLock>
#include <QList>

#include "OsmAndCore.h"
#include <CommonTypes.h>
#include <IMapSymbolProvider.h>

namespace OsmAnd
{
    class OfflineMapDataTile;
    class MapSymbolsGroup;
    class MapSymbolsTile;
    namespace Model {
        class MapObject;
    } // namespace Model

    class OfflineMapSymbolProvider;
    class OfflineMapSymbolProvider_P
    {
    private:
    protected:
        OfflineMapSymbolProvider_P(OfflineMapSymbolProvider* owner);

        OfflineMapSymbolProvider* const owner;

        class Tile : public MapSymbolsTile
        {
        private:
        protected:
        public:
            Tile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups, const std::shared_ptr<const OfflineMapDataTile>& dataTile);
            virtual ~Tile();

            const std::shared_ptr<const OfflineMapDataTile> dataTile;
        };
    public:
        virtual ~OfflineMapSymbolProvider_P();

        bool obtainSymbols(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapSymbolsTile>& outTile,
            std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter);

    friend class OsmAnd::OfflineMapSymbolProvider;
    };

}

#endif // _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_P_H_
