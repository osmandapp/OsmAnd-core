#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_H_

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

    class BinaryMapRasterBitmapTileProvider_P;
    class OSMAND_CORE_API BinaryMapRasterBitmapTileProvider : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(BinaryMapRasterBitmapTileProvider);
    private:
        PrivateImplementation<BinaryMapRasterBitmapTileProvider_P> _p;
    protected:
        BinaryMapRasterBitmapTileProvider(
            BinaryMapRasterBitmapTileProvider_P* const p,
            const std::shared_ptr<BinaryMapDataProvider>& dataProvider,
            const uint32_t tileSize,
            const float densityFactor);
    public:
        virtual ~BinaryMapRasterBitmapTileProvider();

        const std::shared_ptr<BinaryMapDataProvider> dataProvider;
        const uint32_t tileSize;
        const float densityFactor;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<const MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };

    class OSMAND_CORE_API BinaryMapRasterizedTile : public RasterBitmapTile
    {
        Q_DISABLE_COPY(BinaryMapRasterizedTile);

    private:
    protected:
    public:
        BinaryMapRasterizedTile(
            const std::shared_ptr<const BinaryMapDataTile>& binaryMapTile,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const AlphaChannelData alphaChannelData,
            const float densityFactor,
            const TileId tileId,
            const ZoomLevel zoom);
        virtual ~BinaryMapRasterizedTile();

        const std::shared_ptr<const BinaryMapDataTile> binaryMapTile;

        virtual std::shared_ptr<MapData> createNoContentInstance() const;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_H_)
