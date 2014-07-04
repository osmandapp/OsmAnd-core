#include "FavoriteLocationsPresenter_P.h"
#include "FavoriteLocationsPresenter.h"

OsmAnd::FavoriteLocationsPresenter_P::FavoriteLocationsPresenter_P(FavoriteLocationsPresenter* const owner_)
    : owner(owner_)
{
}

OsmAnd::FavoriteLocationsPresenter_P::~FavoriteLocationsPresenter_P()
{
}

bool OsmAnd::FavoriteLocationsPresenter_P::isGroupVisible(const QString& group) const
{
    return true;
}

void OsmAnd::FavoriteLocationsPresenter_P::setIsGroupVisible(const QString& group, const bool isVisible)
{

}

void OsmAnd::FavoriteLocationsPresenter_P::showGroup(const QString& group)
{

}

void OsmAnd::FavoriteLocationsPresenter_P::hideGroup(const QString& group)
{

}

bool OsmAnd::FavoriteLocationsPresenter_P::isFavoriteLocationVisible(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation, const bool checkGroup) const
{
    return true;
}

void OsmAnd::FavoriteLocationsPresenter_P::setIsFavoriteLocationVisible(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation, const bool isVisible)
{

}

void OsmAnd::FavoriteLocationsPresenter_P::showFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{

}

void OsmAnd::FavoriteLocationsPresenter_P::hideFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{

}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::FavoriteLocationsPresenter_P::getProvidedDataKeys() const
{
    return QList<OsmAnd::IMapKeyedSymbolsProvider::Key>();
}

bool OsmAnd::FavoriteLocationsPresenter_P::obtainData(const IMapKeyedSymbolsProvider::Key key, std::shared_ptr<MapKeyedData>& outKeyedData, const IQueryController* const queryController)
{
    return false;
}

void OsmAnd::FavoriteLocationsPresenter_P::subscribeToChanges()
{

}

void OsmAnd::FavoriteLocationsPresenter_P::unsubscribeToChanges()
{

}
