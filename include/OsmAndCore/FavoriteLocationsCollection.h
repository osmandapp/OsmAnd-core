#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IFavoriteLocationsCollection.h>

namespace OsmAnd
{
    class IFavoriteLocation;
    class FavoriteLocation;
    class FavoriteLocation_P;

    class FavoriteLocationsCollection_P;
    class OSMAND_CORE_API FavoriteLocationsCollection : public IFavoriteLocationsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsCollection);

    private:
        PrivateImplementation<FavoriteLocationsCollection_P> _p;
    protected:
        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position31,
            const QString& elevation = QString(),
            const QString& time = QString(),
            const QString& creationTime = QString(),
            const QString& title = QString(),
            const QString& description = QString(),
            const QString& address = QString(),
            const QString& group = QString(),
            const QString& icon = QString(),
            const QString& background = QString(),
            const ColorARGB color = ColorARGB(),
            const QHash<QString, QString>& extensions = QHash<QString, QString>(),
            const bool calendarEvent = false,
            const QString& amenityOriginName = QString());

        FavoriteLocationsCollection(FavoriteLocationsCollection_P* const p);
    public:
        FavoriteLocationsCollection();
        virtual ~FavoriteLocationsCollection();

        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const LatLon latLon,
            const QString& elevation = QString(),
            const QString& time = QString(),
            const QString& creationTime = QString(),
            const QString& title = QString(),
            const QString& description = QString(),
            const QString& address = QString(),
            const QString& group = QString(),
            const QString& icon = QString(),
            const QString& background = QString(),
            const ColorARGB color = ColorARGB(),
            const QHash<QString, QString>& extensions = QHash<QString, QString>(),
            const bool calendarEvent = false,
            const QString& amenityOriginName = QString());

        virtual std::shared_ptr<IFavoriteLocation> copyFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& other);
        virtual void addFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation);
        virtual void addFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocationsm, const bool notifyChanged = true);
        virtual bool removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation);
        virtual bool removeFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations);
        virtual void clearFavoriteLocations();

        virtual unsigned int getFavoriteLocationsCount() const;
        virtual QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const;

        virtual unsigned int getVisibleFavoriteLocationsCount() const;
        virtual QList< std::shared_ptr<IFavoriteLocation> > getVisibleFavoriteLocations() const;

        virtual QSet<QString> getGroups() const;
        virtual QHash<QString, QList<std::shared_ptr<IFavoriteLocation>>> getGroupsLocations() const;

        virtual void copyFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection);
        virtual void copyFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection);
        virtual void copyFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection);
        virtual void mergeFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection);
        virtual void mergeFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection);
        virtual void mergeFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection);

    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocation_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_)
