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

#ifndef __MODEL_STREET_H_
#define __MODEL_STREET_H_

#include <stdint.h>

#include <QString>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    struct ObfAddressSection;

    namespace Model {

        class OSMAND_CORE_API Street
        {
        private:
        protected:
            uint64_t _id;
            QString _name;
            QString _latinName;
            PointI _tile24;
            PointD _location;
            uint32_t _offset;

            Street();
        public:
            virtual ~Street();

            const uint64_t& id;
            const QString& name;
            const QString& latinName;
            const PointI& tile24;
            const PointD& location;
            
            struct IntersectedStreet
            {
                QString _name;
                QString _latinName;

                int _xTile24;
                int _yTile24;
                double _longitude;
                double _latitude;
            };

        friend struct OsmAnd::ObfAddressSection;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // __MODEL_STREET_H_
