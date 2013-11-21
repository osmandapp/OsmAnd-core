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

#ifndef _OSMAND_CORE_MODEL_STREET_GROUP_H_
#define _OSMAND_CORE_MODEL_STREET_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd {

    namespace Model {

        class OSMAND_CORE_API StreetGroup
        {
        private:
        protected:
        public:
            StreetGroup();
            virtual ~StreetGroup();

            int64_t _id;
            QString _name;
            QString _latinName;
            double _longitude;
            double _latitude;
            unsigned int _offset;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // _OSMAND_CORE_MODEL_STREET_GROUP_H_
