/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_MAP_STYLES_H_
#define _OSMAND_CORE_MAP_STYLES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>

namespace OsmAnd {

    class MapStyle;
    class MapStyle_P;

    class MapStyles_P;
    class OSMAND_CORE_API MapStyles
    {
        Q_DISABLE_COPY(MapStyles);
    private:
        const std::unique_ptr<MapStyles_P> _d;
    protected:
    public:
        MapStyles();
        virtual ~MapStyles();

        bool registerStyle(const QString& filePath);
        bool obtainStyle(const QString& name, std::shared_ptr<const OsmAnd::MapStyle>& outStyle);

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyle_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLES_H_
