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

#ifndef __INSPECTOR_H_
#define __INSPECTOR_H_

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCoreUtils.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace Inspector
    {
        struct OSMAND_CORE_UTILS_API Configuration
        {
            Configuration();
            Configuration(const QString& fileName);

            QString fileName;
            bool verboseAddress;
            bool verboseStreetGroups;
            bool verboseStreets;
            bool verboseBuildings;
            bool verboseIntersections;
            bool verboseMap;
            bool verboseMapObjects;
            bool verbosePoi;
            bool verboseAmenities;
            bool verboseTrasport;
            AreaD bbox;
            ZoomLevel zoom;
        };
        OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL dumpToStdOut(const Configuration& cfg);
        OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL dumpToString(const Configuration& cfg);
    } // namespace Inspector

} // namespace OsmAnd 

#endif // __INSPECTOR_H_
