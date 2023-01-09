#ifndef _OSMAND_CORE_GEO_TILE_OBJECTS_PROVIDER_H_
#define _OSMAND_CORE_GEO_TILE_OBJECTS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapObjectsProvider.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>
#include <OsmAndCore/Map/WeatherCommonTypes.h>
#include <OsmAndCore/Map/WeatherTileResourcesManager.h>

namespace OsmAnd
{
    class MapObject;

    class GeoTileObjectsProvider_P;
    class OSMAND_CORE_API GeoTileObjectsProvider Q_DECL_FINAL
        : public std::enable_shared_from_this<GeoTileObjectsProvider>
        , public IMapObjectsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTileObjectsProvider);

    private:
        PrivateImplementation<GeoTileObjectsProvider_P> _p;
        
        const std::shared_ptr<WeatherTileResourcesManager> _resourcesManager;
        
    protected:
    public:
        GeoTileObjectsProvider(
            const std::shared_ptr<WeatherTileResourcesManager> resourcesManager,
            const int64_t dateTime,
            const BandIndex band,
            const bool localData,
            const uint32_t cacheSize = 0);
        virtual ~GeoTileObjectsProvider();

        const int64_t dateTime;
        const BandIndex band;
        const bool localData;
        const uint32_t cacheSize;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

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
        
        friend class OsmAnd::GeoTileObjectsProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_TILE_OBJECTS_PROVIDER_H_)
