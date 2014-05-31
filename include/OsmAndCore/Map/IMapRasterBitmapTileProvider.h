#ifndef _OSMAND_CORE_I_MAP_RASTER_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_RASTER_BITMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapRasterBitmapTileProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY(IMapRasterBitmapTileProvider);
    private:
    protected:
        IMapRasterBitmapTileProvider();
    public:
        virtual ~IMapRasterBitmapTileProvider();

        virtual uint32_t getTileSize() const = 0;
        virtual float getTileDensityFactor() const = 0;
    };

    class OSMAND_CORE_API RasterBitmapTile : public MapTiledData
    {
        Q_DISABLE_COPY(RasterBitmapTile);

    private:
    protected:
    public:
        RasterBitmapTile(
            const std::shared_ptr<const SkBitmap>& bitmap,
            const AlphaChannelData alphaChannelData,
            const float densityFactor,
            const TileId tileId,
            const ZoomLevel zoom);
        virtual ~RasterBitmapTile();

        const std::shared_ptr<const SkBitmap> bitmap;
        const AlphaChannelData alphaChannelData;
        const float densityFactor;

        RasterBitmapTile* cloneWithBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_RASTER_BITMAP_TILE_PROVIDER_H_)
