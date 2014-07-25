#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_BITMAP_TILE_PROVIDER_H_

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
    class BinaryMapPrimitivesProvider;
    class BinaryMapPrimitivesTile;

    class BinaryMapPrimitivesMetricsBitmapTileProvider_P;
    class OSMAND_CORE_API BinaryMapPrimitivesMetricsBitmapTileProvider : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(BinaryMapPrimitivesMetricsBitmapTileProvider);
    private:
        PrivateImplementation<BinaryMapPrimitivesMetricsBitmapTileProvider_P> _p;
    protected:
    public:
        BinaryMapPrimitivesMetricsBitmapTileProvider(
            const std::shared_ptr<BinaryMapPrimitivesProvider>& privitivesProvider,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f);
        virtual ~BinaryMapPrimitivesMetricsBitmapTileProvider();

        const std::shared_ptr<BinaryMapPrimitivesProvider> primitivesProvider;
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

    class OSMAND_CORE_API BinaryMapPrimitivesMetricsTile : public RasterBitmapTile
    {
        Q_DISABLE_COPY(BinaryMapPrimitivesMetricsTile);

    private:
    protected:
    public:
        BinaryMapPrimitivesMetricsTile(
            const std::shared_ptr<const BinaryMapPrimitivesTile>& binaryMapPrimitivesTile,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const float densityFactor,
            const TileId tileId,
            const ZoomLevel zoom);
        virtual ~BinaryMapPrimitivesMetricsTile();

        const std::shared_ptr<const BinaryMapPrimitivesTile> binaryMapPrimitivesTile;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_METRICS_BITMAP_TILE_PROVIDER_H_)
