#ifndef _OSMAND_CORE_OBFS_COLLECTION_P_H_
#define _OSMAND_CORE_OBFS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QHash>
#include <QMultiHash>
#include <QReadWriteLock>
#include <QAtomicInt>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "ObfsCollection.h"

namespace OsmAnd
{
    class ObfFile;
    class ObfDataInterface;

    class ObfsCollection;
    class ObfsCollection_P
    {
    private:
    protected:
        ObfsCollection_P(ObfsCollection* owner);

        ObfsCollection* const owner;

        enum class EntryType
        {
            DirectoryEntry,
            ExplicitFileEntry,
        };
        struct Entry
        {
            Entry(EntryType type_)
                : type(type_)
            {
            }

            const EntryType type;
        };
        struct DirectoryEntry : Entry
        {
            DirectoryEntry()
                : Entry(EntryType::DirectoryEntry)
            {
            }

            QDir dir;
            bool recursive;
        };
        struct ExplicitFileEntry : Entry
        {
            ExplicitFileEntry()
                : Entry(EntryType::ExplicitFileEntry)
            {
            }

            QFileInfo fileInfo;
        };
        QHash< ObfsCollection::EntryId, std::shared_ptr<Entry> > _entries;
        ObfsCollection::EntryId _lastUnusedEntryId;
        mutable QReadWriteLock _entriesLock;

        QHash<void*, ObfsCollection::CollectedSourcesUpdateObserverSignature> _collectedSourcesUpdateObservers;
        mutable QReadWriteLock _collectedSourcesUpdateObserversLock;

        void invalidateCollectedSources();
        QAtomicInt _collectedSourcesInvalidated;
        QHash< ObfsCollection::EntryId, QHash<QString, std::shared_ptr<ObfFile> > > _collectedSources;
        QHash< ObfsCollection::EntryId, QAtomicInt> _collectedSourcesOriginRefCounters;
        mutable QReadWriteLock _collectedSourcesLock;
        void collectSources();

        ObfsCollection::SourcesSetModifierSignature _sourcesSetModifier;
        QReadWriteLock _sourcesSetModifierLock;
    public:
        virtual ~ObfsCollection_P();

        ObfsCollection::EntryId registerDirectory(const QDir& dir, bool recursive = true);
        ObfsCollection::EntryId registerExplicitFile(const QFileInfo& fileInfo);
        bool unregister(const ObfsCollection::EntryId entryId);

        void notifyFileSystemChange();

        QStringList getCollectedSources() const;
        void notifyCollectedSourcesUpdate() const;
        void registerCollectedSourcesUpdateObserver(void* tag, const ObfsCollection::CollectedSourcesUpdateObserverSignature observer);
        void unregisterCollectedSourcesUpdateObserver(void* tag);

        void setSourcesSetModifier(const ObfsCollection::SourcesSetModifierSignature modifier);

        std::shared_ptr<ObfDataInterface> obtainDataInterface();

    friend class OsmAnd::ObfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_P_H_)
