#ifndef _OSMAND_CORE_BINARY_MAP_METRICS_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_METRICS_BITMAP_TILE_PROVIDER_H_

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

namespace OsmAnd
{
    class BinaryMapDataProvider;
    class BinaryMapDataTile;

    class BinaryMapMetricsBitmapTileProvider_P;
    class OSMAND_CORE_API BinaryMapMetricsBitmapTileProvider : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(BinaryMapMetricsBitmapTileProvider);
    private:
        PrivateImplementation<BinaryMapMetricsBitmapTileProvider_P> _p;
    protected:
    public:
        BinaryMapMetricsBitmapTileProvider(
            const std::shared_ptr<BinaryMapDataProvider>& dataProvider,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f);
        virtual ~BinaryMapMetricsBitmapTileProvider();

        const std::shared_ptr<BinaryMapDataProvider> dataProvider;
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

    class OSMAND_CORE_API BinaryMapMetricsTile : public RasterBitmapTile
    {
        Q_DISABLE_COPY(BinaryMapMetricsTile);

    private:
    protected:
    public:
        BinaryMapMetricsTile(
            const std::shared_ptr<const BinaryMapDataTile>& binaryMapTile,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const float densityFactor,
            const TileId tileId,
            const ZoomLevel zoom);
        virtual ~BinaryMapMetricsTile();

        const std::shared_ptr<const BinaryMapDataTile> binaryMapTile;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_METRICS_BITMAP_TILE_PROVIDER_H_)
