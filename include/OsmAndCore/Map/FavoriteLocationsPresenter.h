#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/MapMarker.h>

namespace OsmAnd
{
    class IFavoriteLocation;
    class IFavoriteLocationsCollection;

    class FavoriteLocationsPresenter_P;
    class OSMAND_CORE_API FavoriteLocationsPresenter
        : public std::enable_shared_from_this<FavoriteLocationsPresenter>
        , public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsPresenter);

    private:
        PrivateImplementation<FavoriteLocationsPresenter_P> _p;
    protected:
    public:
        FavoriteLocationsPresenter(
            const std::shared_ptr<const IFavoriteLocationsCollection>& collection,
            const sk_sp<const SkImage>& favoriteLocationPinIcon = nullptr,
            const Nullable<MapMarker::PinIconVerticalAlignment> favoriteLocationPinIconVerticalAlignment = Nullable<MapMarker::PinIconVerticalAlignment>(),
            const Nullable<MapMarker::PinIconHorisontalAlignment> favoriteLocationPinIconHorisontalAlignment = Nullable<MapMarker::PinIconHorisontalAlignment>());
        virtual ~FavoriteLocationsPresenter();

        const std::shared_ptr<const IFavoriteLocationsCollection> collection;
        const sk_sp<const SkImage> favoriteLocationPinIcon;
        const Nullable<MapMarker::PinIconVerticalAlignment> favoriteLocationPinIconVerticalAlignment;
        const Nullable<MapMarker::PinIconHorisontalAlignment> favoriteLocationPinIconHorisontalAlignment;

        static sk_sp<const SkImage> getDefaultFavoriteLocationPinIcon();
        static MapMarker::PinIconVerticalAlignment getDefaultFavoriteLocationPinIconVerticalAlignment();
        static MapMarker::PinIconHorisontalAlignment getDefaultFavoriteLocationPinIconHorisontalAlignment();

        virtual QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;
        
        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_)
