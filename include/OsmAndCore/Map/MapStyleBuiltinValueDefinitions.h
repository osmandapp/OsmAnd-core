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

#ifndef _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_
#define _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd {

    class MapStyleValueDefinition;
    class MapStyle;

    class OSMAND_CORE_API MapStyleBuiltinValueDefinitions
    {
        Q_DISABLE_COPY(MapStyleBuiltinValueDefinitions);
    private:
    protected:
        MapStyleBuiltinValueDefinitions();
    public:
        virtual ~MapStyleBuiltinValueDefinitions();

        // Definitions
#       define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
            const std::shared_ptr<const MapStyleValueDefinition> varname;
#       include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions_Set.h>
#       undef DECLARE_BUILTIN_VALUEDEF

        // Identifiers of definitions above
#       define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
            const int id_##varname;
#       include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions_Set.h>
#       undef DECLARE_BUILTIN_VALUEDEF

    friend class OsmAnd::MapStyle;

    private:
        int _footerDummy;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_
