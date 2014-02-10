#ifndef _OSMAND_CORE_OBFS_COLLECTION_P_H_
#define _OSMAND_CORE_OBFS_COLLECTION_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QHash>
#include <QMutex>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfFile;
    class ObfDataInterface;

    class ObfsCollection;
    class ObfsCollection_P
    {
    private:
    protected:
        ObfsCollection_P(ObfsCollection* owner);

        ObfsCollection* const owner;

        mutable QMutex _watchedCollectionMutex;
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

        mutable QMutex _sourcesMutex;
        QHash< QString, std::shared_ptr<ObfFile> > _sources;
        bool _sourcesRefreshedOnce;
        void refreshSources();
    public:
        virtual ~ObfsCollection_P();

        std::shared_ptr<ObfDataInterface> obtainDataInterface();

    friend class OsmAnd::ObfsCollection;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_P_H_)
