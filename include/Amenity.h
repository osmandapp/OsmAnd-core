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

#ifndef __MODEL_AMENITY_H_
#define __MODEL_AMENITY_H_

#include <OsmAndCore.h>
#include <MapObject.h>
#include <QString>
#include <QStringList>
#include <stdint.h>

namespace OsmAnd {

    namespace Model {

        class OSMAND_CORE_API Amenity : public MapObject
        {
        public:
            struct OSMAND_CORE_API Category
            {
                QString _name;
                QStringList _subcategories;
            };
        private:
        protected:
        public:
            Amenity();
            virtual ~Amenity();

            int64_t _id;
            QString _name;
            QString _latinName;
            uint32_t _x31;
            uint32_t _y31;
            double _longitude;
            double _latitude;
            QString _openingHours;
            QString _site;
            QString _phone;
            QString _description;
            uint32_t _categoryId;
            uint32_t _subcategoryId;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // __MODEL_AMENITY_H_
