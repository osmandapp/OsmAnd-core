#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_METRICS_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_METRICS_BITMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapRasterBitmapTileProvider.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider_Metrics.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider.h>

namespace OsmAnd
{
    class BinaryMapRasterMetricsBitmapTileProvider_P;
    class OSMAND_CORE_API BinaryMapRasterMetricsBitmapTileProvider : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(BinaryMapRasterMetricsBitmapTileProvider);
    private:
        PrivateImplementation<BinaryMapRasterMetricsBitmapTileProvider_P> _p;
    protected:
    public:
        BinaryMapRasterMetricsBitmapTileProvider(
            const std::shared_ptr<BinaryMapRasterBitmapTileProvider>& rasterBitmapTileProvider,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f);
        virtual ~BinaryMapRasterMetricsBitmapTileProvider();

        const std::shared_ptr<BinaryMapRasterBitmapTileProvider> rasterBitmapTileProvider;
        const uint32_t tileSize;
        const float densityFactor;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };

    class OSMAND_CORE_API BinaryMapRasterMetricsBitmapTile : public RasterBitmapTile
    {
        Q_DISABLE_COPY(BinaryMapRasterMetricsBitmapTile);

    private:
    protected:
    public:
        BinaryMapRasterMetricsBitmapTile(
            const std::shared_ptr<const BinaryMapRasterizedTile>& rasterizedMapTile,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const float densityFactor,
            const TileId tileId,
            const ZoomLevel zoom);
        virtual ~BinaryMapRasterMetricsBitmapTile();

        const std::shared_ptr<const BinaryMapRasterizedTile> rasterizedMapTile;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_METRICS_BITMAP_TILE_PROVIDER_H_)
