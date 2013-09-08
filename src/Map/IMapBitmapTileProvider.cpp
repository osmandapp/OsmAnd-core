#include "IMapBitmapTileProvider.h"

#include <cassert>

#include <SkBitmap.h>

// Ensure that SKIA is using RGBA order
#include <SkColor.h>
#ifdef SK_CPU_LENDIAN
#   if (24 != SK_A32_SHIFT) || ( 0 != SK_R32_SHIFT) || \
       ( 8 != SK_G32_SHIFT) || (16 != SK_B32_SHIFT)
#       error SKIA must be configured to use RGBA color order
#   endif
#else
#   if ( 0 != SK_A32_SHIFT) || (24 != SK_R32_SHIFT) || \
       (16 != SK_G32_SHIFT) || ( 8 != SK_B32_SHIFT)
#       error SKIA must be configured to use RGBA color order
#   endif
#endif

OsmAnd::IMapBitmapTileProvider::IMapBitmapTileProvider()
    : IMapTileProvider(MapTileDataType::Bitmap)
{
}

OsmAnd::IMapBitmapTileProvider::~IMapBitmapTileProvider()
{
}

OsmAnd::MapBitmapTile::MapBitmapTile( SkBitmap* bitmap_, const AlphaChannelData& alphaChannelData_ )
    : MapTile(MapTileDataType::Bitmap, bitmap_->getPixels(), bitmap_->rowBytes(), bitmap_->width())
    , _bitmap(bitmap_)
    , bitmap(_bitmap)
    , alphaChannelData(alphaChannelData_)
{
    assert(bitmap_->width() == bitmap_->height());
}

OsmAnd::MapBitmapTile::~MapBitmapTile()
{
}
