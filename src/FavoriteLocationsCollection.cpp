#include "FavoriteLocationsCollection.h"
#include "FavoriteLocationsCollection_P.h"

#include "IFavoriteLocation.h"

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
    const PointI position31,
    const QString& elevation /*= QString::null*/,
    const QString& time /*= QString::null*/,
    const QString& pickupTime /*= QString::null*/,
    const QString& title /*= QString::null*/,
    const QString& description /*= QString::null*/,
    const QString& address /*= QString::null*/,
    const QString& group /*= QString::null*/,
    const QString& icon /*= QString::null*/,
    const QString& background /*= QString::null*/,
    const ColorARGB color /*= ColorARGB()*/,
    const QHash<QString, QString>& extensions, /*= QHash<QString, QString>*/
    const bool calendarEvent /*= false*/,
    const QString& amenityOriginName /*= QString()*/)
{
    return _p->createFavoriteLocation(position31, elevation, time, pickupTime, title, description, address, group, icon, background, color, extensions, calendarEvent, amenityOriginName);
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection::createFavoriteLocation(
    const LatLon latLon,
    const QString& elevation /*= QString::null*/,
    const QString& time /*= QString::null*/,
    const QString& pickupTime /*= QString::null*/,
    const QString& title /*= QString::null*/,
    const QString& description /*= QString::null*/,
    const QString& address /*= QString::null*/,
    const QString& group /*= QString::null*/,
    const QString& icon /*= QString::null*/,
    const QString& background /*= QString::null*/,
    const ColorARGB color /*= ColorARGB()*/,
    const QHash<QString, QString>& extensions, /*= QHash<QString, QString>*/
    const bool calendarEvent /*= false*/,
    const QString& amenityOriginName /*= QString()*/)
{
    return _p->createFavoriteLocation(latLon, elevation, time, pickupTime, title, description, address, group, icon, background, color, extensions, calendarEvent, amenityOriginName);
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection::copyFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& other)
{
    if (other->getLocationSource() == IFavoriteLocation::LocationSource::Point31)
    {
        return _p->createFavoriteLocation(
            other->getPosition31(),
            other->getElevation(),
            other->getTime(),
            other->getPickupTime(),
            other->getTitle(),
            other->getDescription(),
            other->getAddress(),
            other->getGroup(),
            other->getIcon(),
            other->getBackground(),
            other->getColor(),
            other->getExtensions(),
            other->getCalendarEvent(),
	        other->getAmenityOriginName());
    }
    else //if (other->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
    {
        return _p->createFavoriteLocation(
            other->getLatLon(),
            other->getElevation(),
            other->getTime(),
            other->getPickupTime(),
            other->getTitle(),
            other->getDescription(),
            other->getAddress(),
            other->getGroup(),
            other->getIcon(),
            other->getBackground(),
            other->getColor(),
            other->getExtensions(),
            other->getCalendarEvent(),
	        other->getAmenityOriginName());
    }
}

void OsmAnd::FavoriteLocationsCollection::addFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation)
{
    _p->addFavoriteLocation(favoriteLocation);
}

void OsmAnd::FavoriteLocationsCollection::addFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations, const bool notifyChanged /*= true*/)
{
    _p->addFavoriteLocations(favoriteLocations, notifyChanged);
}

bool OsmAnd::FavoriteLocationsCollection::removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation)
{
    return _p->removeFavoriteLocation(favoriteLocation);
}

bool OsmAnd::FavoriteLocationsCollection::removeFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations)
{
    return _p->removeFavoriteLocations(favoriteLocations);
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

unsigned int OsmAnd::FavoriteLocationsCollection::getVisibleFavoriteLocationsCount() const
{
    return _p->getVisibleFavoriteLocationsCount();
}

QList< std::shared_ptr<OsmAnd::IFavoriteLocation> > OsmAnd::FavoriteLocationsCollection::getVisibleFavoriteLocations() const
{
    return _p->getVisibleFavoriteLocations();
}

QSet<QString> OsmAnd::FavoriteLocationsCollection::getGroups() const
{
    return _p->getGroups();
}

QHash<QString, QList<std::shared_ptr<OsmAnd::IFavoriteLocation>>> OsmAnd::FavoriteLocationsCollection::getGroupsLocations() const
{
    return _p->getGroupsLocations();
}

void OsmAnd::FavoriteLocationsCollection::copyFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection)
{
    _p->copyFrom(otherCollection->getFavoriteLocations());
}

void OsmAnd::FavoriteLocationsCollection::copyFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection)
{
    _p->copyFrom(otherCollection);
}

void OsmAnd::FavoriteLocationsCollection::copyFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection)
{
    _p->copyFrom(otherCollection);
}

void OsmAnd::FavoriteLocationsCollection::mergeFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection)
{
    _p->mergeFrom(otherCollection->getFavoriteLocations());
}

void OsmAnd::FavoriteLocationsCollection::mergeFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection)
{
    _p->mergeFrom(otherCollection);
}

void OsmAnd::FavoriteLocationsCollection::mergeFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection)
{
    _p->mergeFrom(otherCollection);
}
