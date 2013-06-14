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

#include <stdint.h>

#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class ObfPoiSection;
    class PoiDirectory;

    namespace Model {

        class OSMAND_CORE_API Amenity
        {
        public:
            class OSMAND_CORE_API Category
            {
            private:
            protected:
                Category();

                QString _name;
                QStringList _subcategories;
            public:
                virtual ~Category();

                const QString& name;
                const QStringList& subcategories;

            friend class OsmAnd::ObfPoiSection;
            friend class OsmAnd::PoiDirectory;
            };
        private:
        protected:
            Amenity();

            uint64_t _id;
            QString _name;
            QString _latinName;
            PointI _point31;
            QString _openingHours;
            QString _site;
            QString _phone;
            QString _description;
            uint32_t _categoryId;
            uint32_t _subcategoryId;
        public:
            virtual ~Amenity();

            const uint64_t& id;
            const QString& name;
            const QString& latinName;
            const PointI& point31;
            const QString& openingHours;
            const QString& site;
            const QString& phone;
            const QString& description;
            const uint32_t& categoryId;
            const uint32_t& subcategoryId;

        friend class OsmAnd::ObfPoiSection;
        friend class OsmAnd::PoiDirectory;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // __MODEL_AMENITY_H_
