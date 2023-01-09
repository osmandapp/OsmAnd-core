#include "FavoriteLocationsPresenter.h"
#include "FavoriteLocationsPresenter_P.h"

#include "CoreResourcesEmbeddedBundle.h"
#include "MapDataProviderHelpers.h"

OsmAnd::FavoriteLocationsPresenter::FavoriteLocationsPresenter(
    const std::shared_ptr<const IFavoriteLocationsCollection>& collection_,
    const sk_sp<const SkImage>& favoriteLocationPinIcon_ /*= nullptr*/,
    const Nullable<MapMarker::PinIconVerticalAlignment> favoriteLocationPinIconVerticalAlignment_ /*= Nullable<MapMarker::PinIconVerticalAlignment>()*/,
    const Nullable<MapMarker::PinIconHorisontalAlignment> favoriteLocationPinIconHorisontalAlignment_ /*= Nullable<MapMarker::PinIconHorisontalAlignment>()*/)
    : _p(new FavoriteLocationsPresenter_P(this))
    , collection(collection_)
    , favoriteLocationPinIcon(favoriteLocationPinIcon_)
    , favoriteLocationPinIconVerticalAlignment(favoriteLocationPinIconVerticalAlignment_)
    , favoriteLocationPinIconHorisontalAlignment(favoriteLocationPinIconHorisontalAlignment_)
{
    _p->subscribeToChanges();
    _p->syncFavoriteLocationMarkers();
}

OsmAnd::FavoriteLocationsPresenter::~FavoriteLocationsPresenter()
{
    _p->unsubscribeToChanges();
}

sk_sp<const SkImage> OsmAnd::FavoriteLocationsPresenter::getDefaultFavoriteLocationPinIcon()
{
    static const sk_sp<const SkImage> defaultFavoriteLocationPinIcon(
        getCoreResourcesProvider()->getResourceAsImage(QLatin1String("map/stubs/favorite_location_pin_icon.png")));
    return defaultFavoriteLocationPinIcon;
}

OsmAnd::MapMarker::PinIconVerticalAlignment OsmAnd::FavoriteLocationsPresenter::getDefaultFavoriteLocationPinIconVerticalAlignment()
{
    return static_cast<OsmAnd::MapMarker::PinIconVerticalAlignment>(MapMarker::PinIconVerticalAlignment::Bottom);
}

OsmAnd::MapMarker::PinIconHorisontalAlignment OsmAnd::FavoriteLocationsPresenter::getDefaultFavoriteLocationPinIconHorisontalAlignment()
{
    return static_cast<OsmAnd::MapMarker::PinIconHorisontalAlignment>(MapMarker::PinIconHorisontalAlignment::CenterHorizontal);
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::FavoriteLocationsPresenter::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::FavoriteLocationsPresenter::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::FavoriteLocationsPresenter::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::FavoriteLocationsPresenter::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::FavoriteLocationsPresenter::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::FavoriteLocationsPresenter::getMinZoom() const
{
    return OsmAnd::ZoomLevel5;
}

OsmAnd::ZoomLevel OsmAnd::FavoriteLocationsPresenter::getMaxZoom() const
{
    return OsmAnd::MaxZoomLevel;
}
