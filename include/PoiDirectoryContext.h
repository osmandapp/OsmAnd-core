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

#ifndef __POI_DIRECTORY_CONTEXT_H_
#define __POI_DIRECTORY_CONTEXT_H_

#include <limits>
#include <memory>

#include <QString>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QList>

#include <OsmAndCore.h>
#include <ObfReader.h>

namespace OsmAnd {

    class PoiDirectory;

    class OSMAND_CORE_API PoiDirectoryContext
    {
    private:
    protected:
        struct PoiSectionContext
        {
            QList< std::shared_ptr<Model::Amenity::Category> > _categories;
            struct PoiCategoryData
            {
                uint32_t _id;
                QHash<QString, uint32_t> _subcategoriesIds;
            };
            QHash<QString, std::shared_ptr<PoiCategoryData>> _categoriesIds;
        };
        QMap< std::shared_ptr<ObfPoiSection>, std::shared_ptr<PoiSectionContext> > _contexts;

        QMultiHash< QString, QString > _categoriesCache;
    public:
        PoiDirectoryContext(const QList< std::shared_ptr<ObfReader> >& sources);
        virtual ~PoiDirectoryContext();

        const QList< std::shared_ptr<ObfReader> > sources;

        friend class OsmAnd::PoiDirectory;
    };

} // namespace OsmAnd

#endif // __POI_DIRECTORY_CONTEXT_H_
