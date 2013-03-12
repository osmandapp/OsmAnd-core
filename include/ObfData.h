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

#ifndef __OBF_DATA_H_
#define __OBF_DATA_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <QMap>
#include <QList>
#include <QMultiHash>
#include <QMutex>

#include <OsmAndCore.h>
#include <ObfReader.h>
#include <Amenity.h>


namespace OsmAnd {

    class OSMAND_CORE_API ObfData
    {
    private:
    protected:
        QMutex _sourcesMutex;
        struct Data
        {
            struct PoiData
            {
                QList< std::shared_ptr<Model::Amenity::Category> > _categories;
                struct PoiCategoryData
                {
                    uint32_t _id;
                    QHash<QString, uint32_t> _subcategoriesIds;
                };
                QHash<QString, std::shared_ptr<PoiCategoryData>> _categoriesIds;
            };
            QMap< std::shared_ptr<ObfPoiSection>, std::shared_ptr<PoiData> > _poiData;
        };
        QMap< std::shared_ptr<ObfReader>, std::shared_ptr<Data> > _sources;

        QMultiHash< QString, QString > _combinedPoiCategories;
        volatile bool _combinedPoiCategoriesInvalidated;
        void invalidateCombinedPoiCategories();
    public:
        ObfData();
        virtual ~ObfData();

        void addSource(const std::shared_ptr<OsmAnd::ObfReader>& source);
        void removeSource(const std::shared_ptr<OsmAnd::ObfReader>& source);
        void clearSources();
        QList< std::shared_ptr<OsmAnd::ObfReader> > getSources();
        
        QMultiHash< QString, QString > getPoiCategories();
        void queryPoiAmenities(
            QMultiHash< QString, QString >* desiredCategories = nullptr,
            QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut = nullptr,
            IQueryFilter* filter = nullptr, uint32_t zoomToSkipFilter = 3,
            std::function<bool (std::shared_ptr<OsmAnd::Model::Amenity>)>* visitor = nullptr,
            IQueryController* controller = nullptr);
    };

} // namespace OsmAnd

#endif // __OBF_DATA_H_
