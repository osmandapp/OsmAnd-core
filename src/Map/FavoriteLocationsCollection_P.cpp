#include "FavoriteLocationsCollection_P.h"
#include "FavoriteLocationsCollection.h"

OsmAnd::FavoriteLocationsCollection_P::FavoriteLocationsCollection_P(FavoriteLocationsCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::FavoriteLocationsCollection_P::~FavoriteLocationsCollection_P()
{
}

void OsmAnd::FavoriteLocationsCollection_P::notifyCollectionChanged()
{
}

void OsmAnd::FavoriteLocationsCollection_P::notifyFavoriteLocationChanged(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection_P::createFavoriteLocation(const PointI position, const QString& title, const QString& group, const ColorRGB color)
{

}

bool OsmAnd::FavoriteLocationsCollection_P::removeFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{

}

void OsmAnd::FavoriteLocationsCollection_P::clearFavoriteLocations()
{

}

unsigned int OsmAnd::FavoriteLocationsCollection_P::getFavoriteLocationsCount() const
{

}

QList< std::shared_ptr<OsmAnd::IFavoriteLocation> > OsmAnd::FavoriteLocationsCollection_P::getFavoriteLocations() const
{

}
