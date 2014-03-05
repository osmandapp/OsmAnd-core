#ifndef _OSMAND_CORE_OBFS_COLLECTION_H_
#define _OSMAND_CORE_OBFS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

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
    private:
    protected:
        const std::unique_ptr<ObfsCollection_P> _d;
    public:
        ObfsCollection();
        virtual ~ObfsCollection();

        void watchDirectory(const QDir& dir, bool recursive = true);
        void watchDirectory(const QString& dirPath, bool recursive = true);
        void registerExplicitFile(const QFileInfo& fileInfo);
        void registerExplicitFile(const QString& filePath);

        std::shared_ptr<ObfDataInterface> obtainDataInterface() const;
    };
}

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_H_)
