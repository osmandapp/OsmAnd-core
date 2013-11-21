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
#ifndef _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_H_
#define _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapSymbolProvider.h>

namespace OsmAnd {

    class OfflineMapDataProvider;

    class OfflineMapSymbolProvider_P;
    class OSMAND_CORE_API OfflineMapSymbolProvider : public IMapSymbolProvider
    {
        Q_DISABLE_COPY(OfflineMapSymbolProvider);
    private:
        const std::unique_ptr<OfflineMapSymbolProvider_P> _d;
    protected:
    public:
        OfflineMapSymbolProvider(const std::shared_ptr<OfflineMapDataProvider>& dataProvider, const float density = 1.0f);
        virtual ~OfflineMapSymbolProvider();

        const std::shared_ptr<OfflineMapDataProvider> dataProvider;

        virtual bool obtainSymbols(const TileId tileId, const ZoomLevel zoom, QList< std::shared_ptr<const MapSymbol> >& outSymbols);
    };

}

#endif // _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_H_
