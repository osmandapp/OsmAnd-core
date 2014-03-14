#ifndef _OSMAND_CORE_OBFS_COLLECTION_H_
#define _OSMAND_CORE_OBFS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class ObfDataInterface;
    class ObfsCollection_P;
    class OSMAND_CORE_API ObfsCollection
    {
        Q_DISABLE_COPY(ObfsCollection);
    public:
        typedef int EntryId;
        typedef std::function<void (ObfsCollection&)> CollectedSourcesUpdateObserverSignature;

    private:
    protected:
        const std::unique_ptr<ObfsCollection_P> _d;
    public:
        ObfsCollection();
        virtual ~ObfsCollection();

        EntryId registerDirectory(const QDir& dir, bool recursive = true);
        EntryId registerDirectory(const QString& dirPath, bool recursive = true);
        EntryId registerExplicitFile(const QFileInfo& fileInfo);
        EntryId registerExplicitFile(const QString& filePath);
        bool unregister(const EntryId entryId);

        void notifyFileSystemChange() const;

        QStringList getCollectedSources() const;

        void registerCollectedSourcesUpdateObserver(void* tag, const CollectedSourcesUpdateObserverSignature observer) const;
        void unregisterCollectedSourcesUpdateObserver(void* tag) const;

        std::shared_ptr<ObfDataInterface> obtainDataInterface() const;
    };
}

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_H_)
