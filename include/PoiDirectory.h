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

#ifndef __POI_DIRECTORY_H_
#define __POI_DIRECTORY_H_

#include <limits>
#include <memory>

#include <QString>
#include <QMultiHash>
#include <QList>

#include <OsmAndCore.h>
#include <PoiDirectoryContext.h>

namespace OsmAnd {

    class OSMAND_CORE_API PoiDirectory
    {
    private:
    protected:
        PoiDirectory();
    public:
        virtual ~PoiDirectory();

        static const QMultiHash< QString, QString >& getPoiCategories(PoiDirectoryContext* context);
        static void queryPoiAmenities(
            PoiDirectoryContext* context,
            QMultiHash< QString, QString >* desiredCategories = nullptr,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut = nullptr,
            QueryFilter* filter = nullptr, uint32_t zoomToSkipFilter = 3,
            std::function<bool (std::shared_ptr<OsmAnd::Model::Amenity>)> visitor = nullptr,
            IQueryController* controller = nullptr);
    };

} // namespace OsmAnd

#endif // __POI_DIRECTORY_H_
