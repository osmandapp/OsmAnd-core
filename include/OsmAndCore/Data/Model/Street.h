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

#include <cstdint>

#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfAddressSectionReader_P;

    namespace Model {

        class OSMAND_CORE_API Street
        {
        private:
        protected:
            uint64_t _id;
            QString _name;
            QString _latinName;
            PointI _tile24;
            uint32_t _offset;

            Street();
        public:
            virtual ~Street();

            const uint64_t& id;
            const QString& name;
            const QString& latinName;
            const PointI& tile24;

        friend class OsmAnd::ObfAddressSectionReader_P;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // __MODEL_STREET_H_
