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

#ifndef __MAP_STYLE_RULE_H_
#define __MAP_STYLE_RULE_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class MapStyle;
    class MapStyle_P;
    class MapStyleEvaluator;
    class MapStyleValueDefinition;
    struct MapStyleValue;

    class MapStyleRule_P;
    class OSMAND_CORE_API MapStyleRule
    {
        Q_DISABLE_COPY(MapStyleRule);
    private:
        const std::unique_ptr<MapStyleRule_P> _d;
    protected:
        MapStyleRule(MapStyle* owner, const QHash< QString, QString >& attributes);

        bool getAttribute(const QString& key, std::shared_ptr<const MapStyleValue>& value) const;
    public:
        virtual ~MapStyleRule();

        MapStyle* const owner;

        void dump(const QString& prefix = QString()) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_RULE_H_
