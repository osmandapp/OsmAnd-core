#include "FavoriteLocationsCollection.h"
#include "FavoriteLocationsCollection_P.h"

OsmAnd::FavoriteLocationsCollection::FavoriteLocationsCollection()
    : _p(new FavoriteLocationsCollection_P(this))
{
}

OsmAnd::FavoriteLocationsCollection::~FavoriteLocationsCollection()
{
}

void OsmAnd::FavoriteLocationsCollection::notifyCollectionChanged()
{
    _p->notifyCollectionChanged();
}

void OsmAnd::FavoriteLocationsCollection::notifyFavoriteLocationChanged(const std::shared_ptr<const FavoriteLocation>& favoriteLocation)
{
    _p->notifyFavoriteLocationChanged(favoriteLocation);
}

std::shared_ptr<OsmAnd::FavoriteLocation> OsmAnd::FavoriteLocationsCollection::createFavoriteLocation(
    const PointI position,
    const QString& title /*= QString::null*/,
    const QString& group /*= QString::null*/,
    const ColorRGB color /*= ColorRGB()*/)
{
    return _p->createFavoriteLocation(position, title, group, color);
}

bool OsmAnd::FavoriteLocationsCollection::removeFavoriteLocation(const std::shared_ptr<const FavoriteLocation>& favoriteLocation)
{
    return _p->removeFavoriteLocation(favoriteLocation);
}

void OsmAnd::FavoriteLocationsCollection::clearFavoriteLocations()
{
    _p->clearFavoriteLocations();
}

unsigned int OsmAnd::FavoriteLocationsCollection::getFavoriteLocationsCount() const
{
    return _p->getFavoriteLocationsCount();
}

QList< std::shared_ptr<OsmAnd::FavoriteLocation> > OsmAnd::FavoriteLocationsCollection::getFavoriteLocations() const
{
    return _p->getFavoriteLocations();
}
