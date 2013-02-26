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

#ifndef __INSPECTOR_H_
#define __INSPECTOR_H_

#ifdef OSMAND_INSPECTOR_EXPORTS
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_INSPECTOR_API \
            __declspec(dllexport)
#       define OSMAND_INSPECTOR_CALL \
            __stdcall
#   else
#       if !defined(__arm__)
#           define OSMAND_INSPECTOR_CALL
#       else
#           define OSMAND_INSPECTOR_CALL
#       endif
#       if __GNUC__ >= 4
#           define OSMAND_INSPECTOR_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_INSPECTOR_API
#       endif
#   endif
#   define OSMAND_INSPECTOR_API_DL \
        OSMAND_INSPECTOR_API
#else
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_INSPECTOR_API \
            __declspec(dllimport)
#       define OSMAND_INSPECTOR_CALL \
            __stdcall
#   else
#       if !defined(__arm__)
#           define OSMAND_INSPECTOR_CALL
#       else
#           define OSMAND_INSPECTOR_CALL
#       endif
#       if __GNUC__ >= 4
#           define OSMAND_INSPECTOR_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_INSPECTOR_API
#       endif
#   endif
#   define OSMAND_INSPECTOR_API_DL \
        OSMAND_INSPECTOR_API
#endif

#include <QString>
#include <QStringList>

namespace OsmAnd
{
    namespace Inspector
    {
        struct OSMAND_INSPECTOR_API Configuration
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
            bool verbosePoi;
            bool verboseAmenities;
            bool verboseTrasport;
            double latTop;
            double latBottom;
            double lonLeft;
            double lonRight;
            int zoom;
        };
        OSMAND_INSPECTOR_API bool OSMAND_INSPECTOR_CALL parseCommandLineArguments(QStringList cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_INSPECTOR_API void OSMAND_INSPECTOR_CALL dumpToStdOut(Configuration cfg);
        OSMAND_INSPECTOR_API QString OSMAND_INSPECTOR_CALL dumpToString(Configuration cfg);
    } // namespace Inspector

} // namespace OsmAnd 

#endif // __INSPECTOR_H_
