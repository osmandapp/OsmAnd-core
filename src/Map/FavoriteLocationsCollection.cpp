#include "FavoriteLocationsCollection.h"
#include "FavoriteLocationsCollection_P.h"

OsmAnd::FavoriteLocationsCollection::FavoriteLocationsCollection()
	: FavoriteLocationsCollection(new FavoriteLocationsCollection_P(this))
{
}

OsmAnd::FavoriteLocationsCollection::FavoriteLocationsCollection(FavoriteLocationsCollection_P* const p)
	: _p(p)
{
}

OsmAnd::FavoriteLocationsCollection::~FavoriteLocationsCollection()
{
	_p->_containerLink->release();
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection::createFavoriteLocation(
    const PointI position,
    const QString& title /*= QString::null*/,
    const QString& group /*= QString::null*/,
    const ColorRGB color /*= ColorRGB()*/)
{
    return _p->createFavoriteLocation(position, title, group, color);
}

bool OsmAnd::FavoriteLocationsCollection::removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation)
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

QList< std::shared_ptr<OsmAnd::IFavoriteLocation> > OsmAnd::FavoriteLocationsCollection::getFavoriteLocations() const
{
    return _p->getFavoriteLocations();
}

QSet<QString> OsmAnd::FavoriteLocationsCollection::getGroups() const
{
    return _p->getGroups();
}
