#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    class IFavoriteLocation;
    class FavoriteLocation;
    class FavoriteLocation_P;

    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY(FavoriteLocationsCollection_P);

    private:
    protected:
        FavoriteLocationsCollection_P(FavoriteLocationsCollection* const owner);

        void notifyCollectionChanged();
        void notifyFavoriteLocationChanged(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation);
    public:
        ~FavoriteLocationsCollection_P();

        ImplementationInterface<FavoriteLocationsCollection> owner;

        std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position,
            const QString& title,
            const QString& group,
            const ColorRGB color);
        bool removeFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation);
        void clearFavoriteLocations();

        unsigned int getFavoriteLocationsCount() const;
        QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const;

    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocation_P;
    };
}

#endif // !defined(_OSMAND_CORE_I_FAVORITE_LOCATIONS_COLLECTION_H_)
