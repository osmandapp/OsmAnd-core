#include "FavoriteLocationsPresenter.h"
#include "FavoriteLocationsPresenter_P.h"

#include "CoreResourcesEmbeddedBundle.h"

OsmAnd::FavoriteLocationsPresenter::FavoriteLocationsPresenter(
    const std::shared_ptr<const IFavoriteLocationsCollection>& collection_,
    const std::shared_ptr<const SkBitmap>& favoriteLocationPinIconBitmap_ /*= nullptr*/)
    : _p(new FavoriteLocationsPresenter_P(this))
    , collection(collection_)
    , favoriteLocationPinIconBitmap(favoriteLocationPinIconBitmap_)
{
    _p->subscribeToChanges();
    _p->syncFavoriteLocationMarkers();
}

OsmAnd::FavoriteLocationsPresenter::~FavoriteLocationsPresenter()
{
    _p->unsubscribeToChanges();
}

std::shared_ptr<const SkBitmap> OsmAnd::FavoriteLocationsPresenter::getDefaultFavoriteLocationPinIconBitmap()
{
    static const std::shared_ptr<const SkBitmap> defaultFavoriteLocationPinIconBitmap(
        getCoreResourcesProvider()->getResourceAsBitmap(QLatin1String("map/stubs/favorite_location_pin_icon.png")));
    return defaultFavoriteLocationPinIconBitmap;
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::FavoriteLocationsPresenter::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::FavoriteLocationsPresenter::obtainData(
    const IMapKeyedDataProvider::Key key,
    std::shared_ptr<IMapKeyedDataProvider::Data>& outKeyedData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(key, outKeyedData, queryController);
}
