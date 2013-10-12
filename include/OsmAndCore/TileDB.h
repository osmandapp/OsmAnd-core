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
#ifndef __TILE_DB_H_
#define __TILE_DB_H_

#include <cstdint>
#include <memory>
#include <array>

#include <QDir>
#include <QFile>
#include <QString>
#include <QMutex>
#include <QSqlDatabase>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class OSMAND_CORE_API TileDB
    {
    public:
    private:
    protected:
        QMutex _indexMutex;
        QSqlDatabase _indexDb;

        bool openIndex();
    public:
        TileDB(const QDir& dataPath, const QString& indexFilename = QString());
        virtual ~TileDB();

        const QDir dataPath;
        const QString indexFilename;

        bool rebuildIndex();
        bool obtainTileData(const TileId tileId, const ZoomLevel zoom, QByteArray& data);
    };

}

#endif // __TILE_DB_H_
