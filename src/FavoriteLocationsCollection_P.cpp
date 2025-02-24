#include "FavoriteLocationsCollection_P.h"
#include "FavoriteLocationsCollection.h"

#include "FavoriteLocation.h"

OsmAnd::FavoriteLocationsCollection_P::FavoriteLocationsCollection_P(FavoriteLocationsCollection* const owner_)
    : _containerLink(new Link(owner_))
    , owner(owner_)
{
}

OsmAnd::FavoriteLocationsCollection_P::~FavoriteLocationsCollection_P()
{
}

void OsmAnd::FavoriteLocationsCollection_P::notifyCollectionChanged()
{
    owner->collectionChangeObservable.postNotify(owner);
}

void OsmAnd::FavoriteLocationsCollection_P::notifyFavoriteLocationChanged(FavoriteLocation* const pFavoriteLocation)
{
    QReadLocker scopedLocker(&_collectionLock);

    owner->favoriteLocationChangeObservable.postNotify(owner, constOf(_collection)[pFavoriteLocation]);
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection_P::createFavoriteLocation(
    const PointI position,
    const QString& elevation,
    const QString& time,
    const QString& pickupTime,
    const QString& title,
    const QString& description,
    const QString& address,
    const QString& group,
    const QString& icon,
    const QString& background,
    const ColorARGB color,
    const QHash<QString, QString>& extensions,
    const bool calendarEvent,
	const QString& amenityOriginName)
{
    QWriteLocker scopedLocker(&_collectionLock);

    std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(position, elevation, time, pickupTime, title, description, address, group, icon, background, color, extensions, calendarEvent, amenityOriginName));

    return newItem;
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection_P::createFavoriteLocation(
    const LatLon latLon,
    const QString& elevation,
    const QString& time,
    const QString& pickupTime,
    const QString& title,
    const QString& description,
    const QString& address,
    const QString& group,
    const QString& icon,
    const QString& background,
    const ColorARGB color,
    const QHash<QString, QString>& extensions,
    const bool calendarEvent,
	const QString& amenityOriginName)
{
    QWriteLocker scopedLocker(&_collectionLock);

    std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(latLon, elevation, time, pickupTime, title, description, address, group, icon, background, color, extensions, calendarEvent, amenityOriginName));

    return newItem;
}

void OsmAnd::FavoriteLocationsCollection_P::addFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation)
{
    QWriteLocker scopedLocker(&_collectionLock);

    const auto _favoriteLocation = std::static_pointer_cast<FavoriteLocation>(favoriteLocation);
    _favoriteLocation->attach(_containerLink);
    _collection.insert(_favoriteLocation.get(), _favoriteLocation);

    notifyCollectionChanged();
}

void OsmAnd::FavoriteLocationsCollection_P::addFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations, const bool notifyChanged /*= true*/)
{
    QWriteLocker scopedLocker(&_collectionLock);

    for (const auto& favoriteLocation : favoriteLocations)
    {
        const auto _favoriteLocation = std::static_pointer_cast<FavoriteLocation>(favoriteLocation);
        _favoriteLocation->attach(_containerLink);
        _collection.insert(_favoriteLocation.get(), _favoriteLocation);
    }

    if (notifyChanged)
        notifyCollectionChanged();
}

bool OsmAnd::FavoriteLocationsCollection_P::removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation)
{
    QWriteLocker scopedLocker(&_collectionLock);

    const auto item = std::static_pointer_cast<FavoriteLocation>(favoriteLocation);
    item->detach();
    const auto removedCount = _collection.remove(item.get());
    const auto wasRemoved = (removedCount > 0);

    if (wasRemoved)
        notifyCollectionChanged();

    return wasRemoved;
}

bool OsmAnd::FavoriteLocationsCollection_P::removeFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations)
{
    QWriteLocker scopedLocker(&_collectionLock);

    int removedCount = 0;
    for (const auto& favoriteLocation : favoriteLocations)
    {
        const auto item = std::static_pointer_cast<FavoriteLocation>(favoriteLocation);
        item->detach();
        removedCount += _collection.remove(item.get());
    }

    const auto wasRemoved = (removedCount > 0);
    if (wasRemoved)
        notifyCollectionChanged();

    return wasRemoved;
}

void OsmAnd::FavoriteLocationsCollection_P::clearFavoriteLocations()
{
    QWriteLocker scopedLocker(&_collectionLock);

    doClearFavoriteLocations();

    notifyCollectionChanged();
}

unsigned int OsmAnd::FavoriteLocationsCollection_P::getFavoriteLocationsCount() const
{
    QReadLocker scopedLocker(&_collectionLock);

    return _collection.size();
}

QList< std::shared_ptr<OsmAnd::IFavoriteLocation> > OsmAnd::FavoriteLocationsCollection_P::getFavoriteLocations() const
{
    QReadLocker scopedLocker(&_collectionLock);

    return copyAs< QList< std::shared_ptr<IFavoriteLocation> > >(_collection.values());
}

unsigned int OsmAnd::FavoriteLocationsCollection_P::getVisibleFavoriteLocationsCount() const
{
    QReadLocker scopedLocker(&_collectionLock);

    unsigned int res = 0;
    for (const auto& item : _collection)
        res += item->isHidden() ? 0 : 1;

    return res;
}

QList< std::shared_ptr<OsmAnd::IFavoriteLocation> > OsmAnd::FavoriteLocationsCollection_P::getVisibleFavoriteLocations() const
{
    QReadLocker scopedLocker(&_collectionLock);

    QList< std::shared_ptr<IFavoriteLocation> > res;
    for (const auto& item : _collection)
        if (!item->isHidden())
            res.push_back(item);

    return res;
}

QSet<QString> OsmAnd::FavoriteLocationsCollection_P::getGroups() const
{
    QReadLocker scopedLocker(&_collectionLock);

    QSet<QString> result;
    for (const auto& item : constOf(_collection))
    {
        const auto& group = item->getGroup();
        result.insert(group);
    }

    return result;
}

QHash<QString, QList<std::shared_ptr<OsmAnd::IFavoriteLocation>>> OsmAnd::FavoriteLocationsCollection_P::getGroupsLocations() const
{
    QReadLocker scopedLocker(&_collectionLock);

    QHash<QString, QList<std::shared_ptr<IFavoriteLocation>>> result;
    for (const auto& item : constOf(_collection))
    {
        const auto& group = item->getGroup();
        auto& locations = result[group];
        locations << item;
    }
    return result;
}

void OsmAnd::FavoriteLocationsCollection_P::doClearFavoriteLocations()
{
    // Detach all items from this collection to avoid invalid notifications
    for (const auto& item : _collection)
        item->detach();

    _collection.clear();
}

void OsmAnd::FavoriteLocationsCollection_P::appendFrom(const QList< std::shared_ptr<FavoriteLocation> >& collection)
{
    for (const auto& item : collection)
    {
        item->attach(_containerLink);
        _collection.insert(item.get(), item);
    }
}

void OsmAnd::FavoriteLocationsCollection_P::copyFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection)
{
    QWriteLocker scopedLocker(&_collectionLock);

    doClearFavoriteLocations();
    for (const auto& item : otherCollection)
    {
        if (item->getLocationSource() == IFavoriteLocation::LocationSource::Point31)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
    }

    notifyCollectionChanged();
}

void OsmAnd::FavoriteLocationsCollection_P::copyFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection)
{
    QWriteLocker scopedLocker(&_collectionLock);

    doClearFavoriteLocations();
    for (const auto& item : otherCollection)
    {
        if (item->getLocationSource() == IFavoriteLocation::LocationSource::Point31)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
    }

    notifyCollectionChanged();
}

void OsmAnd::FavoriteLocationsCollection_P::mergeFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection)
{
    QWriteLocker scopedLocker(&_collectionLock);

    for (const auto& item : otherCollection)
    {
        if (item->getLocationSource() == IFavoriteLocation::LocationSource::Point31)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
    }

    notifyCollectionChanged();
}

void OsmAnd::FavoriteLocationsCollection_P::mergeFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection)
{
    QWriteLocker scopedLocker(&_collectionLock);

    for (const auto& item : otherCollection)
    {
        if (item->getLocationSource() == IFavoriteLocation::LocationSource::Point31)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getPickupTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getExtensions(),
                item->getCalendarEvent(),
                item->getAmenityOriginName()));
            newItem->attach(_containerLink);
            _collection.insert(newItem.get(), newItem);
        }
    }

    notifyCollectionChanged();
}
