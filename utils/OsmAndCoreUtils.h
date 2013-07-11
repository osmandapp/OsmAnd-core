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

#ifndef __CORE_UTILS_H_
#define __CORE_UTILS_H_

#ifdef OSMAND_CORE_UTILS_EXPORTS
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_UTILS_API \
    __declspec(dllexport)
#       define OSMAND_CORE_UTILS_CALL \
    __stdcall
#   else
#       if !defined(__arm__)
#           define OSMAND_CORE_UTILS_CALL
#       else
#           define OSMAND_CORE_UTILS_CALL
#       endif
#       if __GNUC__ >= 4
#           define OSMAND_CORE_UTILS_API \
    __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_UTILS_API
#       endif
#   endif
#   define OSMAND_CORE_UTILS_API_DL \
    OSMAND_CORE_UTILS_API
#else
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_UTILS_API \
    __declspec(dllimport)
#       define OSMAND_CORE_UTILS_CALL \
    __stdcall
#   else
#       if !defined(__arm__)
#           define OSMAND_CORE_UTILS_CALL
#       else
#           define OSMAND_CORE_UTILS_CALL
#       endif
#       if __GNUC__ >= 4
#           define OSMAND_CORE_UTILS_API \
    __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_UTILS_API
#       endif
#   endif
#   define OSMAND_CORE_UTILS_API_DL \
    OSMAND_CORE_UTILS_API
#endif

#endif // __CORE_UTILS_H_
