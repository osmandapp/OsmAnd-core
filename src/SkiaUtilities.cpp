#include "SkiaUtilities.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkBitmapDevice.h>
#include "restore_internal_warnings.h"

OsmAnd::SkiaUtilities::SkiaUtilities()
{
}

OsmAnd::SkiaUtilities::~SkiaUtilities()
{
}

std::shared_ptr<SkBitmap> OsmAnd::SkiaUtilities::scaleBitmap(
    const std::shared_ptr<const SkBitmap>& original,
    float xScale,
    float yScale)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto scaledWidth = qCeil(original->width() * xScale);
    const auto scaledHeight = qCeil(original->height() * yScale);

    if (scaledWidth <= 0 || scaledHeight <= 0)
        return nullptr;
 
    const std::shared_ptr<SkBitmap> scaledBitmap(new SkBitmap());
    scaledBitmap->setConfig(original->config(), scaledWidth, scaledHeight);
    scaledBitmap->allocPixels();
    scaledBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(*scaledBitmap);
    canvas.scale(xScale, yScale);
    canvas.drawBitmap(*original, 0, 0, NULL);
    canvas.flush();

    return scaledBitmap;
}
