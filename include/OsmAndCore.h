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
#if !defined(__OSMAND_CORE_H_)
#define __OSMAND_CORE_H_

#if defined(OSMAND_CORE_EXPORTS)
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_API \
            __declspec(dllexport)
#       define OSMAND_CORE_CALL \
            __stdcall
#   else
#       if !defined(__arm__)
#           define OSMAND_CORE_CALL
#       else
#           define OSMAND_CORE_CALL
#       endif
#       if __GNUC__ >= 4
#           define OSMAND_CORE_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_API
#       endif
#   endif
#   define OSMAND_CORE_API_DL \
        OSMAND_CORE_API
#elif !defined(OSMAND_CORE_STATIC)
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_API \
            __declspec(dllimport)
#       define OSMAND_CORE_CALL \
            __stdcall
#   else
#       if !defined(__arm__)
#           define OSMAND_CORE_CALL
#       else
#           define OSMAND_CORE_CALL
#       endif
#       if __GNUC__ >= 4
#           define OSMAND_CORE_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_API
#       endif
#   endif
#   define OSMAND_CORE_API_DL \
        OSMAND_CORE_API
#else
#   define OSMAND_CORE_CALL
#   define OSMAND_CORE_API_DL
#   define OSMAND_CORE_API
#endif

namespace OsmAnd {
    OSMAND_CORE_API void OSMAND_CORE_CALL InitializeCore();
    OSMAND_CORE_API void OSMAND_CORE_CALL ReleaseCore();
}

#endif // __OSMAND_CORE_H_
