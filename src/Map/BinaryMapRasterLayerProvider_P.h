#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_P_H_

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
#include "BinaryMapRasterLayerProvider.h"
#include "BinaryMapRasterLayerProvider_Metrics.h"

namespace OsmAnd
{
    class MapRasterizer;

    class BinaryMapRasterLayerProvider_P
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterLayerProvider_P);
    private:
    protected:
        BinaryMapRasterLayerProvider_P(BinaryMapRasterLayerProvider* const owner);

        virtual void initialize();
        std::unique_ptr<MapRasterizer> _mapRasterizer;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapPrimitivesRetainableCacheMetadata;
        };

        virtual std::shared_ptr<SkBitmap> rasterize(
            const TileId tileId,
            const ZoomLevel zoom,
            const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& primitivesTile,
            BinaryMapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController) = 0;
    public:
        virtual ~BinaryMapRasterLayerProvider_P();

        ImplementationInterface<BinaryMapRasterLayerProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapRasterLayerProvider::Data>& outTiledData,
            BinaryMapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::BinaryMapRasterLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_P_H_)
