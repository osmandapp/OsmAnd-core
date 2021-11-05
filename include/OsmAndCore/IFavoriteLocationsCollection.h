#ifndef _OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_
#define _OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>

namespace OsmAnd
{
    class IFavoriteLocation;

    class OSMAND_CORE_API IFavoriteLocationsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(IFavoriteLocationsCollection);

    private:
    protected:
        IFavoriteLocationsCollection();
    public:
        virtual ~IFavoriteLocationsCollection();

        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position31,
            const QString& elevation = QString::null,
            const QString& time = QString::null,
            const QString& creationTime = QString::null,
            const QString& title = QString::null,
            const QString& description = QString::null,
            const QString& address = QString::null,
            const QString& group = QString::null,
            const QString& icon = QString::null,
            const QString& background = QString::null,
            const ColorRGB color = ColorRGB()) = 0;
        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const LatLon latLon,
            const QString& elevation = QString::null,
            const QString& time = QString::null,
            const QString& creationTime = QString::null,
            const QString& title = QString::null,
            const QString& description = QString::null,
            const QString& address = QString::null,
            const QString& group = QString::null,
            const QString& icon = QString::null,
            const QString& background = QString::null,
            const ColorRGB color = ColorRGB()) = 0;
        
        virtual std::shared_ptr<IFavoriteLocation> copyFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& other) = 0;
        virtual bool removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation) = 0;
        virtual bool removeFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations) = 0;
        virtual void clearFavoriteLocations() = 0;

        virtual unsigned int getFavoriteLocationsCount() const = 0;
        virtual QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const = 0;

        virtual QSet<QString> getGroups() const = 0;

        virtual void copyFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection) = 0;
        virtual void copyFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection) = 0;
        virtual void copyFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection) = 0;
        virtual void mergeFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection) = 0;
        virtual void mergeFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection) = 0;
        virtual void mergeFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection) = 0;

        OSMAND_OBSERVER_CALLABLE(CollectionChanged,
            const IFavoriteLocationsCollection* const collection);
        const ObservableAs<CollectionChanged> collectionChangeObservable;

        OSMAND_OBSERVER_CALLABLE(FavoriteLocationChanged,
            const IFavoriteLocationsCollection* const collection,
            const std::shared_ptr<const IFavoriteLocation> favoriteLocation);
        const ObservableAs<IFavoriteLocationsCollection::FavoriteLocationChanged> favoriteLocationChangeObservable;
    };
}

#endif // !defined(_OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_)
