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
    const QString& creationTime,
    const QString& title,
    const QString& description,
    const QString& address,
    const QString& group,
    const QString& icon,
    const QString& background,
    const ColorRGB color,
    const bool calendarEvent)
{
    QWriteLocker scopedLocker(&_collectionLock);

    std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(_containerLink, position, elevation, time, creationTime, title, description, address, group, icon, background, color, calendarEvent));
    _collection.insert(newItem.get(), newItem);

    notifyCollectionChanged();

    return newItem;
}

std::shared_ptr<OsmAnd::IFavoriteLocation> OsmAnd::FavoriteLocationsCollection_P::createFavoriteLocation(
    const LatLon latLon,
    const QString& elevation,
    const QString& time,
    const QString& creationTime,
    const QString& title,
    const QString& description,
    const QString& address,
    const QString& group,
    const QString& icon,
    const QString& background,
    const ColorRGB color,
    const bool calendarEvent)
{
    QWriteLocker scopedLocker(&_collectionLock);

    std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(_containerLink, latLon, elevation, time, creationTime, title, description, address, group, icon, background, color, calendarEvent));
    _collection.insert(newItem.get(), newItem);

    notifyCollectionChanged();

    return newItem;
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

QSet<QString> OsmAnd::FavoriteLocationsCollection_P::getGroups() const
{
    QReadLocker scopedLocker(&_collectionLock);

    QSet<QString> result;
    for (const auto& item : constOf(_collection))
    {
        const auto& group = item->getGroup();
        if (group.isEmpty())
            continue;
        result.insert(group);
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
                _containerLink,
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                _containerLink,
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
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
                _containerLink,
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                _containerLink,
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
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
                _containerLink,
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                _containerLink,
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
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
                _containerLink,
                item->getPosition31(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
            _collection.insert(newItem.get(), newItem);
        }
        else //if (item->getLocationSource() == IFavoriteLocation::LocationSource::LatLon)
        {
            std::shared_ptr<FavoriteLocation> newItem(new FavoriteLocation(
                _containerLink,
                item->getLatLon(),
                item->getElevation(),
                item->getTime(),
                item->getCreationTime(),
                item->getTitle(),
                item->getDescription(),
                item->getAddress(),
                item->getGroup(),
                item->getIcon(),
                item->getBackground(),
                item->getColor(),
                item->getCalendarEvent()));
            _collection.insert(newItem.get(), newItem);
        }
    }

    notifyCollectionChanged();
}
