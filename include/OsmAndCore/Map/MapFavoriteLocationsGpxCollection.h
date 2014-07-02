#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IFavoriteLocationsCollection.h>

namespace OsmAnd
{
    class IFavoriteLocation;
    class FavoriteLocation;
    class FavoriteLocation_P;

    class FavoriteLocationsCollection_P;
    class OSMAND_CORE_API FavoriteLocationsCollection : public IFavoriteLocationsCollection
    {
        Q_DISABLE_COPY(FavoriteLocationsCollection);

    private:
        PrivateImplementation<FavoriteLocationsCollection_P> _p;
    protected:
        virtual void notifyCollectionChanged();
        virtual void notifyFavoriteLocationChanged(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation);
    public:
        FavoriteLocationsCollection();
        virtual ~FavoriteLocationsCollection();

        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position,
            const QString& title = QString::null,
            const QString& group = QString::null,
            const ColorRGB color = ColorRGB());
        virtual bool removeFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation);
        virtual void clearFavoriteLocations();

        virtual unsigned int getFavoriteLocationsCount() const;
        virtual QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const;

        friend class OsmAnd::FavoriteLocation;
        friend class OsmAnd::FavoriteLocation_P;
    };
}

#endif // !defined(_OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_)
