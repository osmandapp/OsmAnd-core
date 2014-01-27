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

#ifndef _OSMAND_CORE_MAP_STYLE_CONFIGURABLE_INPUT_VALUE_H_
#define _OSMAND_CORE_MAP_STYLE_CONFIGURABLE_INPUT_VALUE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyleValueDefinition.h>

namespace OsmAnd {

    class MapStyle_P;

    class OSMAND_CORE_API MapStyleConfigurableInputValue : public MapStyleValueDefinition
    {
        Q_DISABLE_COPY(MapStyleConfigurableInputValue);
    private:
    protected:
        MapStyleConfigurableInputValue(const MapStyleValueDataType dataType, const QString& name, const QString& title, const QString& description, const QStringList& possibleValues);
    public:
        virtual ~MapStyleConfigurableInputValue();

        const QString title;
        const QString description;
        const QStringList possibleValues;

    friend class OsmAnd::MapStyle_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLE_CONFIGURABLE_INPUT_VALUE_H_
