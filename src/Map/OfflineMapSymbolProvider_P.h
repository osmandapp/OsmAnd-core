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
#ifndef __OFFLINE_MAP_SYMBOL_PROVIDER_P_H_
#define __OFFLINE_MAP_SYMBOL_PROVIDER_P_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <QSet>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class MapSymbol;

    class OfflineMapSymbolProvider;
    class OfflineMapSymbolProvider_P
    {
    private:
    protected:
        OfflineMapSymbolProvider_P(OfflineMapSymbolProvider* owner);

        OfflineMapSymbolProvider* const owner;
    public:
        virtual ~OfflineMapSymbolProvider_P();

        bool obtainSymbols(const QSet<TileId>& tileIds, const ZoomLevel zoom,
            QList< std::shared_ptr<const MapSymbol> >& outSymbols,
            std::function<bool (const uint64_t)> filterById = nullptr);

    friend class OsmAnd::OfflineMapSymbolProvider;
    };

}

#endif // __OFFLINE_MAP_SYMBOL_PROVIDER_P_H_
