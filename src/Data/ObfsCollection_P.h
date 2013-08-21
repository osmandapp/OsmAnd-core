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

#ifndef __OBFS_COLLECTION_P_H_
#define __OBFS_COLLECTION_P_H_

#include <stdint.h>
#include <memory>

#include <QDir>
#include <QHash>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfFile;

    class ObfsCollection;
    class ObfsCollection_P
    {
    private:
    protected:
        ObfsCollection_P(ObfsCollection* owner);

        ObfsCollection* const owner;

        QMutex _watchedCollectionMutex;
        struct WatchEntry
        {
            enum Type
            {
                WatchedDirectory,
                ExplicitFile,
            };

            WatchEntry(Type type_)
                : type(type_)
            {
            }

            const Type type;
        };
        struct WatchedDirectoryEntry : WatchEntry
        {
            WatchedDirectoryEntry()
                : WatchEntry(WatchedDirectory)
            {
            }

            QDir dir;
            bool recursive;
        };
        struct ExplicitFileEntry : WatchEntry
        {
            ExplicitFileEntry()
                : WatchEntry(ExplicitFile)
            {
            }

            QFileInfo fileInfo;
        };
        QList< std::shared_ptr<WatchEntry> > _watchedCollection;
        bool _watchedCollectionChanged;

        QMutex _sourcesMutex;
        QHash< QString, std::shared_ptr<ObfFile> > _sources;
        bool _sourcesRefreshedOnce;
        void refreshSources();
    public:
        virtual ~ObfsCollection_P();

    friend class OsmAnd::ObfsCollection;
    };

} // namespace OsmAnd

#endif // __OBFS_COLLECTION_P_H_
