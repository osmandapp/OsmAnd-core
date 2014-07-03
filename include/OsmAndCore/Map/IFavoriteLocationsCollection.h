#ifndef _OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_
#define _OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class IFavoriteLocation;

    class OSMAND_CORE_API IFavoriteLocationsCollection
    {
        Q_DISABLE_COPY(IFavoriteLocationsCollection);

    private:
    protected:
        IFavoriteLocationsCollection();
    public:
        virtual ~IFavoriteLocationsCollection();

        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position,
            const QString& title = QString::null,
            const QString& group = QString::null,
            const ColorRGB color = ColorRGB()) = 0;
        virtual bool removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation) = 0;
        virtual void clearFavoriteLocations() = 0;

        virtual unsigned int getFavoriteLocationsCount() const = 0;
        virtual QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const = 0;

        OSMAND_CALLABLE(CollectionChanged, void, const IFavoriteLocationsCollection* const collection);
        const ObservableAs<CollectionChanged> collectionChangeObservable;

        OSMAND_CALLABLE(FavoriteLocationChanged, void, const IFavoriteLocationsCollection* const collection, const std::shared_ptr<const IFavoriteLocation>& favoriteLocation);
        const ObservableAs<FavoriteLocationChanged> favoriteLocationChangeObservable;
    };
}

#endif // !defined(_OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_)
