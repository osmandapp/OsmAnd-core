#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_GPX_COLLECTION_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_GPX_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "FavoriteLocationsCollection_P.h"

namespace OsmAnd
{
    class FavoriteLocationsGpxCollection;
    class FavoriteLocationsGpxCollection_P Q_DECL_FINAL : public FavoriteLocationsCollection_P
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsGpxCollection_P);

    private:
        void writeExtension(const QString &key, const QString &value, QXmlStreamWriter &writer) const;
    protected:
        FavoriteLocationsGpxCollection_P(FavoriteLocationsGpxCollection* const owner);
    public:
        virtual ~FavoriteLocationsGpxCollection_P();

        ImplementationInterface<FavoriteLocationsGpxCollection> owner;

        bool saveTo(const QString& fileName) const;
        bool saveTo(const QString& fileName, const QString& groupName) const;
        bool extracted(const QHash<QString, QString>::const_iterator &extension, QXmlStreamWriter &writer) const;
        
        bool saveTo(QXmlStreamWriter& writer) const;
        bool saveTo(QXmlStreamWriter& writer, const QString& groupName) const;
        bool loadFrom(const QString& fileName, bool append = false);
        bool loadFrom(QXmlStreamReader& reader, bool append = false);
        
        void backup(const QString& backupFile, const QString& externalFile) const;
        void clearOldBackups(const QString& basePath) const;
        QString getBackupFile(const QString& basePath) const;

    friend class OsmAnd::FavoriteLocationsGpxCollection;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_GPX_COLLECTION_P_H_)
