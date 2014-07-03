#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "Link.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    class IFavoriteLocation;
    class FavoriteLocation;
    class FavoriteLocation_P;

    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P
    {
        Q_DISABLE_COPY(FavoriteLocationsCollection_P);

    private:
    protected:
        FavoriteLocationsCollection_P(FavoriteLocationsCollection* const owner);

		Link<FavoriteLocationsCollection*> _containerLink;

		mutable QReadWriteLock _collectionLock;
		QHash< FavoriteLocation*, std::shared_ptr<FavoriteLocation> > _collection;

        void notifyCollectionChanged();
        void notifyFavoriteLocationChanged(FavoriteLocation* const pFavoriteLocation);

		void doClearFavoriteLocations();
    public:
        virtual ~FavoriteLocationsCollection_P();

        ImplementationInterface<FavoriteLocationsCollection> owner;

        std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position,
            const QString& title,
            const QString& group,
            const ColorRGB color);
        bool removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation);
        void clearFavoriteLocations();

        unsigned int getFavoriteLocationsCount() const;
        QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const;

    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocation_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_)
