#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_GPX_COLLECTION_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_GPX_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/FavoriteLocationsCollection.h>

namespace OsmAnd
{
    class FavoriteLocationsGpxCollection_P;
    class OSMAND_CORE_API FavoriteLocationsGpxCollection : public FavoriteLocationsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsGpxCollection);

    private:
        FavoriteLocationsGpxCollection_P* _p;

        FavoriteLocationsGpxCollection(FavoriteLocationsGpxCollection_P* const p);
    protected:
    public:
        FavoriteLocationsGpxCollection();
        virtual ~FavoriteLocationsGpxCollection();

        bool saveTo(const QString& fileName) const;
        bool saveTo(const QString& fileName, const QString& groupName) const;
        bool saveTo(QXmlStreamWriter& writer) const;
        bool saveTo(QXmlStreamWriter& writer, const QString& groupName) const;
        bool loadFrom(const QString& fileName, bool append = false);
        bool loadFrom(QXmlStreamReader& reader, bool append = false);

        static std::shared_ptr<FavoriteLocationsGpxCollection> tryLoadFrom(const QString& filename);
        static std::shared_ptr<FavoriteLocationsGpxCollection> tryLoadFrom(QXmlStreamReader& reader);
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_GPX_COLLECTION_H_)
