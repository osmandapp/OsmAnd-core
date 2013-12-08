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

#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfsCollection;
    class MapStyle;
    class RasterizerEnvironment;
    class RasterizerSharedContext;
    class OfflineMapDataTile;
    class IExternalResourcesProvider;

    class OfflineMapDataProvider_P;
    class OSMAND_CORE_API OfflineMapDataProvider
    {
        Q_DISABLE_COPY(OfflineMapDataProvider);
    private:
        const std::unique_ptr<OfflineMapDataProvider_P> _d;
    protected:
    public:
        OfflineMapDataProvider(const std::shared_ptr<ObfsCollection>& obfsCollection, const std::shared_ptr<const MapStyle>& mapStyle, const float displayDensityFactor, const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider = nullptr);
        virtual ~OfflineMapDataProvider();

        const std::shared_ptr<ObfsCollection> obfsCollection;
        const std::shared_ptr<const MapStyle> mapStyle;
        const std::shared_ptr<RasterizerEnvironment> rasterizerEnvironment;
        const std::shared_ptr<RasterizerSharedContext> rasterizerSharedContext;

        void obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile) const;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_H_
