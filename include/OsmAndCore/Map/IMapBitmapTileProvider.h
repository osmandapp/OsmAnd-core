#ifndef _OSMAND_CORE_I_MAP_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_BITMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/IMapTileProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapBitmapTile : public MapTile
    {
        Q_DISABLE_COPY(MapBitmapTile);
    private:
    protected:
        std::shared_ptr<const SkBitmap> _bitmap;
    public:
        MapBitmapTile(const SkBitmap* const bitmap, const AlphaChannelData alphaChannelData);
        MapBitmapTile(const std::shared_ptr<const SkBitmap>& bitmap, const AlphaChannelData alphaChannelData);
        virtual ~MapBitmapTile();

        const std::shared_ptr<const SkBitmap>& bitmap;

        const AlphaChannelData alphaChannelData;
    };

    class OSMAND_CORE_API IMapBitmapTileProvider : public IMapTileProvider
    {
        Q_DISABLE_COPY(IMapBitmapTileProvider);
    private:
    protected:
        IMapBitmapTileProvider();
    public:
        virtual ~IMapBitmapTileProvider();

        virtual float getTileDensity() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_BITMAP_TILE_PROVIDER_H_)
