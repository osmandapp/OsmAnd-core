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
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IObfsCollection.h>

namespace OsmAnd
{
    class ObfDataInterface;
    class ObfReader;

    class ObfsCollection_P;
    class OSMAND_CORE_API ObfsCollection : public IObfsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(ObfsCollection);
    public:
        typedef int SourceOriginId;

    private:
    protected:
        PrivateImplementation<ObfsCollection_P> _p;
    public:
        ObfsCollection();
        virtual ~ObfsCollection();

        SourceOriginId addDirectory(const QDir& dir, bool recursive = true);
        SourceOriginId addDirectory(const QString& dirPath, bool recursive = true);
        SourceOriginId addFile(const QFileInfo& fileInfo);
        SourceOriginId addFile(const QString& filePath);
        bool remove(const SourceOriginId entryId);

        virtual QList< std::shared_ptr<const ObfFile> > getObfFiles() const;
        virtual std::shared_ptr<ObfDataInterface> obtainDataInterface() const;
    };
}

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_H_)
