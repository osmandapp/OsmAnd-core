#include "SkiaUtilities.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkBitmapDevice.h>
#include <SkStream.h>
#include <SkTypeface.h>
#include "restore_internal_warnings.h"

#include "Logging.h"

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
    if (!scaledBitmap->tryAllocPixels(original->info().makeWH(scaledWidth, scaledHeight)))
        return nullptr;
    scaledBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(*scaledBitmap);
    canvas.scale(xScale, yScale);
    canvas.drawBitmap(*original, 0, 0, NULL);
    canvas.flush();

    return scaledBitmap;
}

SkTypeface* OsmAnd::SkiaUtilities::createTypefaceFromData(const QByteArray& data)
{
    SkTypeface* typeface = nullptr;

    const auto fontDataStream = new SkMemoryStream(data.constData(), data.length(), true);
    typeface = SkTypeface::CreateFromStream(fontDataStream);
    fontDataStream->unref();
    
    return typeface;
}

std::shared_ptr<SkBitmap> OsmAnd::SkiaUtilities::mergeBitmaps(const QList< std::shared_ptr<const SkBitmap> >& bitmaps)
{
    if (bitmaps.isEmpty())
        return nullptr;

    int maxWidth = 0;
    int maxHeight = 0;
    for (const auto& bitmap : constOf(bitmaps))
    {
        maxWidth = qMax(maxWidth, bitmap->width());
        maxHeight = qMax(maxHeight, bitmap->height());
    }

    if (maxWidth <= 0 || maxHeight <= 0)
        return nullptr;

    const std::shared_ptr<SkBitmap> outputBitmap(new SkBitmap());
    if (!outputBitmap->tryAllocPixels(SkImageInfo::MakeN32Premul(maxWidth, maxHeight)))
    {
        return nullptr;
    }
    outputBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkBitmapDevice target(*outputBitmap);
    SkCanvas canvas(&target);
    for (const auto& bitmap : constOf(bitmaps))
    {
        canvas.drawBitmap(*bitmap,
            (maxWidth - bitmap->width()) / 2.0f,
            (maxHeight - bitmap->height()) / 2.0f,
            nullptr);
    }
    canvas.flush();

    return outputBitmap;
}
