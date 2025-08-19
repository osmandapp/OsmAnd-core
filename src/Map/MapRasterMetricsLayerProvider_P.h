#ifndef _OSMAND_CORE_MAP_RASTER_METRICS_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_RASTER_METRICS_LAYER_PROVIDER_P_H_

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
#include "MapRasterMetricsLayerProvider.h"

namespace OsmAnd
{
    class MapRasterMetricsLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterMetricsLayerProvider_P);
    private:
    protected:
        uint32_t tileSize;
        MapRasterMetricsLayerProvider_P(MapRasterMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& rasterizedBinaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> rasterizedBinaryMapRetainableCacheMetadata;
        };
    public:
        ~MapRasterMetricsLayerProvider_P();

        ImplementationInterface<MapRasterMetricsLayerProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::MapRasterMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_METRICS_LAYER_PROVIDER_P_H_)
