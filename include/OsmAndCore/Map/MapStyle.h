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

#ifndef __MAP_STYLE_H_
#define __MAP_STYLE_H_

#include <cstdint>
#include <memory>

#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>

namespace OsmAnd {

    class MapStyles;
    class MapStyles_P;
    class MapStyleEvaluator;

    class MapStyleValueDefinition;
    class MapStyleRule;

    STRONG_ENUM_EX(MapStyleRulesetType, uint32_t)
    {
        Invalid = 0,

        Point = 1,
        Line = 2,
        Polygon = 3,
        Text = 4,
        Order = 5,
    };

    class MapStyle_P;
    class OSMAND_CORE_API MapStyle
    {
    private:
        const std::unique_ptr<MapStyle_P> _d;
    protected:
        MapStyle(MapStyles* styles, const QString& resourcePath, const bool isEmbedded);

        QString _name;
    public:
        virtual ~MapStyle();

        MapStyles* const styles;

        const QString resourcePath;
        const bool isEmbedded;

        const QString& title;

        const QString& name;
        const QString& parentName;
        
        bool isStandalone() const;
        bool areDependenciesResolved() const;

        static const MapStyleBuiltinValueDefinitions builtinValueDefinitions;
        bool resolveValueDefinition(const QString& name, std::shared_ptr<const MapStyleValueDefinition>& outDefinition) const;
        bool resolveAttribute(const QString& name, std::shared_ptr<const MapStyleRule>& outAttribute) const;

        void dump(const QString& prefix = QString()) const;
        void dump(MapStyleRulesetType type, const QString& prefix = QString()) const;

    friend class OsmAnd::MapStyles;
    friend class OsmAnd::MapStyles_P;
    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleRule;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_H_
