#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QSet>
#include <QHash>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapKeyedSymbolsProvider.h"
#include "MapMarkersCollection.h"
#include "FavoriteLocationsPresenter.h"

namespace OsmAnd
{
    class IFavoriteLocation;
    class IFavoriteLocationsCollection;

    class FavoriteLocationsPresenter_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsPresenter_P)

    private:
    protected:
        FavoriteLocationsPresenter_P(FavoriteLocationsPresenter* const owner);

        const std::shared_ptr<MapMarkersCollection> _markersCollection;

        mutable QReadWriteLock _favoriteLocationToMarkerMapLock;
        QHash< std::shared_ptr<const IFavoriteLocation>, std::shared_ptr<MapMarker> > _favoriteLocationToMarkerMap;

        void subscribeToChanges();
        void unsubscribeToChanges();
        void syncFavoriteLocationMarkers();
        void syncFavoriteLocationMarker(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation);
    public:
        ~FavoriteLocationsPresenter_P();

        ImplementationInterface<FavoriteLocationsPresenter> owner;

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::FavoriteLocationsPresenter;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_P_H_)
