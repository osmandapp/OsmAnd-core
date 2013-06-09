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

#ifndef __EYEPIECE_H_
#define __EYEPIECE_H_

#include <memory>

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>

#include "CoreUtils.h"
#include <CommonTypes.h>
#include <RasterizationStyle.h>
#include <RasterizationStyles.h>

namespace OsmAnd
{
    namespace EyePiece
    {
        struct OSMAND_CORE_UTILS_API Configuration
        {
            Configuration();

            bool verbose;
            bool dumpRules;
            bool is32bit;
            bool drawMap;
            bool drawText;
            bool drawIcons;
            QList< std::shared_ptr<QFile> > styleFiles;
            QList< std::shared_ptr<QFile> > obfs;
            QString styleName;
            AreaD bbox;
            uint32_t zoom;
            uint32_t tileSide;
            QString output;
        };
        OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL rasterizeToStdOut(const Configuration& cfg);
        OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL rasterizeToString(const Configuration& cfg);
    } // namespace EyePiece

} // namespace OsmAnd 

#endif // __EYEPIECE_H_
