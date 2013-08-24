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
#ifndef __OBF_DATA_INTERFACE_H_
#define __OBF_DATA_INTERFACE_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    // Forward declarations
    class ObfsCollection;
    class ObfReader;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;

    class ObfDataInterface_P;
    class OSMAND_CORE_API ObfDataInterface
    {
    private:
        const std::unique_ptr<ObfDataInterface_P> _d;
    protected:
        ObfDataInterface(const QList< std::shared_ptr<ObfReader> >& readers);
    public:
        virtual ~ObfDataInterface();

        void obtainBasemapPresenceFlag(bool& basemapPresent, IQueryController* controller = nullptr);
        void obtainMapObjects(QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut, const AreaI& area31, const ZoomLevel& zoom, IQueryController* controller = nullptr);
        
    friend class OsmAnd::ObfsCollection;
    };

}

#endif // __OBF_DATA_INTERFACE_H_
