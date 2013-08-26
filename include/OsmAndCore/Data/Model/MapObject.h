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

#ifndef __MODEL_MAP_OBJECT_H_
#define __MODEL_MAP_OBJECT_H_

#include <stdint.h>
#include <memory>

#include <QList>
#include <QMap>
#include <QVector>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd {

    class ObfMapSectionInfo;
    class ObfMapSectionReader_P;
    class Rasterizer_P;

    namespace Model {

        class OSMAND_CORE_API MapObject
        {
        private:
        protected:
            MapObject(const std::shared_ptr<const ObfMapSectionInfo>& section);

            uint64_t _id;
            MapFoundationType _foundation;
            bool _isArea;
            QVector< PointI > _points31;
            QList< QVector< PointI > > _innerPolygonsPoints31;
            QVector< TagValue > _types;
            QVector< TagValue > _extraTypes;
            QHash< QString, QString > _names;
            AreaI _bbox31;
        public:
            virtual ~MapObject();

            const std::shared_ptr<const ObfMapSectionInfo> section;

            const uint64_t& id;
            const bool& isArea;
            const QVector< PointI >& points31;
            const QList< QVector< PointI > >& innerPolygonsPoints31;
            const QVector< TagValue >& types;
            const QVector< TagValue >& extraTypes;
            const MapFoundationType& foundation;
            const QHash<QString, QString>& names;
            const AreaI& bbox31;

            int getSimpleLayerValue() const;
            bool isClosedFigure(bool checkInner = false) const;

            bool containsType(const QString& tag, const QString& value, bool checkAdditional = false) const;

            size_t calculateApproxConsumedMemory() const;

            friend class OsmAnd::ObfMapSectionReader_P;
            friend class OsmAnd::Rasterizer_P;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // __MODEL_MAP_OBJECT_H_
