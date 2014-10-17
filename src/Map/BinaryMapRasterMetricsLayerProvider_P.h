#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_METRICS_BITMAP_LAYER_P_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_METRICS_BITMAP_LAYER_P_H_

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
#include "BinaryMapRasterMetricsLayerProvider.h"

namespace OsmAnd
{
    class BinaryMapRasterMetricsLayerProvider_P 
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterMetricsLayerProvider_P);
    private:
    protected:
        BinaryMapRasterMetricsLayerProvider_P(BinaryMapRasterMetricsLayerProvider* const owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& rasterizedBinaryMapRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> rasterizedBinaryMapRetainableCacheMetadata;
        };
    public:
        ~BinaryMapRasterMetricsLayerProvider_P();

        ImplementationInterface<BinaryMapRasterMetricsLayerProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<BinaryMapRasterMetricsLayerProvider::Data>& outTiledData,
            const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::BinaryMapRasterMetricsLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_METRICS_BITMAP_LAYER_P_H_)
