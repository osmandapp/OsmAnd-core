#include "FavoriteLocationsPresenter_P.h"
#include "FavoriteLocationsPresenter.h"

#include "QtCommon.h"

#include "MapMarkerBuilder.h"
#include "IFavoriteLocation.h"
#include "IFavoriteLocationsCollection.h"

OsmAnd::FavoriteLocationsPresenter_P::FavoriteLocationsPresenter_P(FavoriteLocationsPresenter* const owner_)
    : owner(owner_)
    , _markersCollection(new MapMarkersCollection())
{
}

OsmAnd::FavoriteLocationsPresenter_P::~FavoriteLocationsPresenter_P()
{
}

bool OsmAnd::FavoriteLocationsPresenter_P::isGroupVisible(const QString& group) const
{
    QReadLocker scopedLocker(&_visibilitySettingsLock);

    return !_hiddenGroups.contains(group);
}

void OsmAnd::FavoriteLocationsPresenter_P::setIsGroupVisible(const QString& group, const bool isVisible)
{
    QWriteLocker scopedLocker(&_visibilitySettingsLock);

    if (isVisible)
        _hiddenGroups.remove(group);
    else
        _hiddenGroups.insert(group);
}

void OsmAnd::FavoriteLocationsPresenter_P::showGroup(const QString& group)
{
    setIsGroupVisible(group, true);
}

void OsmAnd::FavoriteLocationsPresenter_P::hideGroup(const QString& group)
{
    setIsGroupVisible(group, false);
}

bool OsmAnd::FavoriteLocationsPresenter_P::isFavoriteLocationVisible(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation, const bool checkGroup) const
{
    QReadLocker scopedLocker(&_visibilitySettingsLock);

    bool visible = !_hiddenFavoriteLocations.contains(favoriteLocation);
    if (visible && checkGroup)
        visible = !_hiddenGroups.contains(favoriteLocation->getGroup());

    return visible;
}

void OsmAnd::FavoriteLocationsPresenter_P::setIsFavoriteLocationVisible(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation, const bool isVisible)
{
    QWriteLocker scopedLocker(&_visibilitySettingsLock);

    if (isVisible)
        _hiddenFavoriteLocations.remove(favoriteLocation);
    else
        _hiddenFavoriteLocations.insert(favoriteLocation);
}

void OsmAnd::FavoriteLocationsPresenter_P::showFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{
    setIsFavoriteLocationVisible(favoriteLocation, true);
}

void OsmAnd::FavoriteLocationsPresenter_P::hideFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{
    setIsFavoriteLocationVisible(favoriteLocation, false);
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::FavoriteLocationsPresenter_P::getProvidedDataKeys() const
{
    return _markersCollection->getProvidedDataKeys();
}

bool OsmAnd::FavoriteLocationsPresenter_P::obtainData(const IMapKeyedSymbolsProvider::Key key, std::shared_ptr<MapKeyedData>& outKeyedData, const IQueryController* const queryController)
{
    return _markersCollection->obtainData(key, outKeyedData, queryController);
}

void OsmAnd::FavoriteLocationsPresenter_P::subscribeToChanges()
{
    owner->collection->collectionChangeObservable.attach(this,
        [this]
        (const IFavoriteLocationsCollection* const collection)
        {
            syncFavoriteLocationMarkers();
        });
    owner->collection->favoriteLocationChangeObservable.attach(this,
        [this]
        (const IFavoriteLocationsCollection* const collection, const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
        {
            syncFavoriteLocationMarker(favoriteLocation);
        });
}

void OsmAnd::FavoriteLocationsPresenter_P::unsubscribeToChanges()
{
    owner->collection->favoriteLocationChangeObservable.detach(this);
    owner->collection->collectionChangeObservable.detach(this);
}

void OsmAnd::FavoriteLocationsPresenter_P::syncFavoriteLocationMarkers()
{
    QWriteLocker scopedLocker(&_favoriteLocationToMarkerMapLock);

    const auto favoriteLocations = copyAs< QList< std::shared_ptr<const IFavoriteLocation> > >(owner->collection->getFavoriteLocations());

    // Remove all markers that have no corresponding favorite locations anymore
    auto itObsoleteEntry = mutableIteratorOf(_favoriteLocationToMarkerMap);
    while (itObsoleteEntry.hasNext())
    {
        const auto& obsoleteEntry = itObsoleteEntry.next();

        if (favoriteLocations.contains(obsoleteEntry.key()))
            continue;
         
        _markersCollection->removeMarker(obsoleteEntry.value());
        itObsoleteEntry.remove();
    }

    // Create markers for all new favorite locations
    MapMarkerBuilder markerBuilder;
    markerBuilder.setPinIcon(
        owner->favoriteLocationPinIconBitmap
        ? owner->favoriteLocationPinIconBitmap
        : FavoriteLocationsPresenter::getDefaultFavoriteLocationPinIconBitmap());
    for (const auto& favoriteLocation : favoriteLocations)
    {
        if (_favoriteLocationToMarkerMap.contains(favoriteLocation))
            continue;

        markerBuilder.setPosition(favoriteLocation->getPosition());
        markerBuilder.setPinIconModulationColor(favoriteLocation->getColor());

        const auto marker = markerBuilder.buildAndAddToCollection(_markersCollection);
        _favoriteLocationToMarkerMap.insert(favoriteLocation, marker);
    }
}

void OsmAnd::FavoriteLocationsPresenter_P::syncFavoriteLocationMarker(const std::shared_ptr<const IFavoriteLocation>& favoriteLocation)
{
    QReadLocker scopedLocker(&_favoriteLocationToMarkerMapLock);

    const auto citMarker = _favoriteLocationToMarkerMap.constFind(favoriteLocation);
    if (citMarker == _favoriteLocationToMarkerMap.cend())
        return;

    const auto& marker = *citMarker;
    marker->setPinIconModulationColor(favoriteLocation->getColor());
}
