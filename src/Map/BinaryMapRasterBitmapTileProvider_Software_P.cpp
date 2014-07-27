#include "BinaryMapRasterBitmapTileProvider_Software_P.h"
#include "BinaryMapRasterBitmapTileProvider_Software.h"

#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include "BinaryMapPrimitivesProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_Software_P::BinaryMapRasterBitmapTileProvider_Software_P(BinaryMapRasterBitmapTileProvider_Software* owner_)
    : BinaryMapRasterBitmapTileProvider_P(owner_)
    , owner(owner_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_Software_P::~BinaryMapRasterBitmapTileProvider_Software_P()
{
}

bool OsmAnd::BinaryMapRasterBitmapTileProvider_Software_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController)
{
    // Obtain offline map primitives tile
    std::shared_ptr<MapTiledData> primitivesTile_;
    owner->primitivesProvider->obtainData(tileId, zoom, primitivesTile_);
    if (!primitivesTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto primitivesTile = std::static_pointer_cast<BinaryMapPrimitivesTile>(primitivesTile_);

    // Allocate rasterization target
    const auto tileSize = owner->getTileSize();
    const std::shared_ptr<SkBitmap> rasterizationSurface(new SkBitmap());
    rasterizationSurface->setConfig(SkBitmap::kARGB_8888_Config, tileSize, tileSize);
    if (!rasterizationSurface->allocPixels())
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", tileSize, tileSize);
        return false;
    }
    SkBitmapDevice rasterizationTarget(*rasterizationSurface);

    // Create rasterization canvas
    SkCanvas canvas(&rasterizationTarget);

    // Perform actual rendering
    if (!primitivesTile->primitivisedArea->isEmpty())
    {
        _mapRasterizer->rasterize(
            primitivesTile->primitivisedArea,
            canvas,
            true,
            nullptr,
            queryController);
    }
    else
    {
        // If there is no data to rasterize, tell that this tile is not available
        outTiledData.reset();
        return true;
    }

    // Or supply newly rasterized tile
    outTiledData.reset(new BinaryMapRasterizedTile(
        primitivesTile,
        rasterizationSurface,
        AlphaChannelData::NotPresent,
        owner->getTileDensityFactor(),
        tileId,
        zoom));

    return true;
}
