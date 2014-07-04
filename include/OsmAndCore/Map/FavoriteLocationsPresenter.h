#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedDataProvider.h>

namespace OsmAnd
{
    class IFavoriteLocationsCollection;

    class FavoriteLocationsPresenter_P;
    class OSMAND_CORE_API FavoriteLocationsPresenter : public IMapKeyedDataProvider
    {
        Q_DISABLE_COPY(FavoriteLocationsPresenter);

    private:
        PrivateImplementation<FavoriteLocationsPresenter_P> _p;
    protected:
    public:
        FavoriteLocationsPresenter(const std::shared_ptr<const IFavoriteLocationsCollection>& collection);
        virtual ~FavoriteLocationsPresenter();
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_PRESENTER_H_)
