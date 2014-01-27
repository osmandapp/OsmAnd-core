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

#ifndef _OSMAND_CORE_MAP_STYLE_RULE_P_H_
#define _OSMAND_CORE_MAP_STYLE_RULE_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QList>

#include <OsmAndCore.h>
#include <MapStyleValue.h>

namespace OsmAnd {

    class MapStyle_P;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;

    class MapStyleRule;
    class MapStyleRule_P
    {
    private:
    protected:
        MapStyleRule_P(MapStyleRule* owner);

        MapStyleRule* const owner;

        QHash< QString, std::shared_ptr<const MapStyleValueDefinition> > _resolvedValueDefinitions;

        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > _values;
        QList< std::shared_ptr<MapStyleRule> > _ifElseChildren;
        QList< std::shared_ptr<MapStyleRule> > _ifChildren;
    public:
        virtual ~MapStyleRule_P();

    friend class OsmAnd::MapStyleRule;
    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLE_RULE_P_H_
