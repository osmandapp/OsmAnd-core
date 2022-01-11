#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"
#include <QMutex>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IRasterMapLayerProvider.h"
#include "MapRasterLayerProvider.h"
#include "MapRasterLayerProvider_Metrics.h"

namespace OsmAnd
{
    class MapRasterizer;

    class MapRasterLayerProvider_P
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterLayerProvider_P);
    private:
    protected:
        MapRasterLayerProvider_P(MapRasterLayerProvider* const owner);

        virtual void initialize();
        std::unique_ptr<MapRasterizer> _mapRasterizer;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapPrimitivesRetainableCacheMetadata;
        };

        virtual sk_sp<SkImage> rasterize(
            const MapRasterLayerProvider::Request& request,
            const std::shared_ptr<const MapPrimitivesProvider::Data>& primitivesTile,
            MapRasterLayerProvider_Metrics::Metric_obtainData* const metric) = 0;
    public:
        virtual ~MapRasterLayerProvider_P();

        ImplementationInterface<MapRasterLayerProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        bool obtainRasterizedTile(
            const MapRasterLayerProvider::Request& request,
            std::shared_ptr<MapRasterLayerProvider::Data>& outData,
            MapRasterLayerProvider_Metrics::Metric_obtainData* const metric);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::MapRasterLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_P_H_)
