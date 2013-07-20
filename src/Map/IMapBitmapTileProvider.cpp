#include "IMapBitmapTileProvider.h"

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
    : IMapTileProvider(IMapTileProvider::Bitmap)
{
}

OsmAnd::IMapBitmapTileProvider::~IMapBitmapTileProvider()
{
}

OsmAnd::IMapBitmapTileProvider::Tile::Tile( const void* data, size_t rowLength, uint32_t width, uint32_t height, const BitmapFormat& format_ )
    : IMapTileProvider::Tile(IMapTileProvider::Bitmap, data, rowLength, width, height)
    , format(format_)
{
}

OsmAnd::IMapBitmapTileProvider::Tile::~Tile()
{
}
