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

#ifndef __QT_EXTENSIONS_H_
#define __QT_EXTENSIONS_H_

#include <memory>

#include <QtGlobal>
#include <OsmAndCore/CommonTypes.h>
#ifndef SWIG
inline uint qHash(const OsmAnd::TileId value, uint seed = 0) Q_DECL_NOTHROW;

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value, uint seed = 0) Q_DECL_NOTHROW;
#endif

#include <QHash>

#ifndef SWIG
inline uint qHash(const OsmAnd::TileId value, uint seed) Q_DECL_NOTHROW
{
    return ::qHash(value.id, seed);
}

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value, uint seed) Q_DECL_NOTHROW
{
    return ::qHash(value.get(), seed);
}
#endif

#endif // __QT_EXTENSIONS_H_
