#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/MapMarker.h>

class SkBitmap;

namespace OsmAnd
{
    class IFavoriteLocation;
    class IFavoriteLocationsCollection;

    class FavoriteLocationsPresenter_P;
    class OSMAND_CORE_API FavoriteLocationsPresenter : public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsPresenter)

    private:
        PrivateImplementation<FavoriteLocationsPresenter_P> _p;
    protected:
    public:
        FavoriteLocationsPresenter(
            const std::shared_ptr<const IFavoriteLocationsCollection>& collection,
            const std::shared_ptr<const SkBitmap>& favoriteLocationPinIconBitmap = nullptr,
            const Nullable<MapMarker::PinIconAlignment> favoriteLocationPinIconAlignment = Nullable<MapMarker::PinIconAlignment>());
        virtual ~FavoriteLocationsPresenter();

        const std::shared_ptr<const IFavoriteLocationsCollection> collection;
        const std::shared_ptr<const SkBitmap> favoriteLocationPinIconBitmap;
        const Nullable<MapMarker::PinIconAlignment> favoriteLocationPinIconAlignment;

        static std::shared_ptr<const SkBitmap> getDefaultFavoriteLocationPinIconBitmap();
        static MapMarker::PinIconAlignment getDefaultFavoriteLocationPinIconAlignment();

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
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_)
