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

#ifndef _OSMAND_CORE_OSMAND_COMMON_H_
#define _OSMAND_CORE_OSMAND_COMMON_H_

#include <cassert>
#include <memory>
#include <iostream>

#if OSMAND_DEBUG
#   define OSMAND_ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion '" #condition "' failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            assert((condition)); \
        } \
    } while (false)
#else
#   define OSMAND_ASSERT(condition, message)
#endif

#if defined(TEXT) && defined(_T)
#   define xT(x) _T(x)
#else
#   if defined(_UNICODE) || defined(UNICODE)
#       define xT(x) L##x
#   else
#       define xT(x) x
#   endif
#endif

#if defined(_UNICODE) || defined(UNICODE)
#   define QStringToStlString(x) (x).toStdWString()
#else
#   define QStringToStlString(x) (x).toStdString()
#endif

#define REPEAT_UNTIL(exp) \
    for(;!(exp);)

#endif // _OSMAND_CORE_OSMAND_COMMON_H_
