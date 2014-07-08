#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>

namespace OsmAnd
{
    class IFavoriteLocation;
    class IFavoriteLocationsCollection;

    class FavoriteLocationsPresenter_P;
    class OSMAND_CORE_API FavoriteLocationsPresenter : public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY(FavoriteLocationsPresenter);

    private:
        PrivateImplementation<FavoriteLocationsPresenter_P> _p;
    protected:
    public:
        FavoriteLocationsPresenter(
            const std::shared_ptr<const IFavoriteLocationsCollection>& collection,
            const std::shared_ptr<const SkBitmap>& favoriteLocationPinIconBitmap = nullptr);
        virtual ~FavoriteLocationsPresenter();

        const std::shared_ptr<const IFavoriteLocationsCollection> collection;
        const std::shared_ptr<const SkBitmap> favoriteLocationPinIconBitmap;

        static std::shared_ptr<const SkBitmap> getDefaultFavoriteLocationPinIconBitmap();

        virtual QList<Key> getProvidedDataKeys() const;
        virtual bool obtainData(
            const Key key,
            std::shared_ptr<MapKeyedData>& outKeyedData,
            const IQueryController* const queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_)
