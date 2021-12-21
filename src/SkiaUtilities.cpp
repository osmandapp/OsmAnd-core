#include "SkiaUtilities.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include <SkBitmap.h>
#include <SkTypeface.h>
#include <SkCanvas.h>
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
    const float xScale,
    const float yScale)
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
    canvas.drawImage(original->asImage(), 0, 0);
    canvas.flush();

    return scaledBitmap;
}

std::shared_ptr<SkBitmap> OsmAnd::SkiaUtilities::offsetBitmap(
    const std::shared_ptr<const SkBitmap>& original,
    const float xOffset,
    const float yOffset)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto newWidth = original->width() + qAbs(xOffset);
    const auto newHeight = original->height() + qAbs(yOffset);

    auto imageInfo = original->info().makeWH(newWidth, newHeight);

    const std::shared_ptr<SkBitmap> newBitmap(new SkBitmap());
    if (!newBitmap->tryAllocPixels(imageInfo))
        return nullptr;
    newBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(*newBitmap);
    canvas.drawImage(original->asImage(), xOffset > 0.0f ? xOffset : 0, yOffset > 0.0f ? yOffset : 0);
    canvas.flush();

    return newBitmap;
}

std::shared_ptr<SkBitmap> OsmAnd::SkiaUtilities::createTileBitmap(
    const std::shared_ptr<const SkBitmap>& firstBitmap,
    const std::shared_ptr<const SkBitmap>& secondBitmap,
    const float yOffset)
{
    SkImageInfo imageInfo;
    if (firstBitmap && firstBitmap->width() > 0 && firstBitmap->height() > 0)
        imageInfo = firstBitmap->info();

    if (secondBitmap && secondBitmap->width() > 0 && secondBitmap->height() > 0)
        imageInfo = secondBitmap->info();

    if (imageInfo.isEmpty())
        return nullptr;

    const std::shared_ptr<SkBitmap> newBitmap(new SkBitmap());

    if (!newBitmap->tryAllocPixels(imageInfo))
        return nullptr;
    newBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(*newBitmap);
    if (firstBitmap)
        canvas.drawImage(firstBitmap->asImage(), 0, -yOffset);
    if (secondBitmap)
        canvas.drawImage(secondBitmap->asImage(), 0, -yOffset + imageInfo.height());
    canvas.flush();

    return newBitmap;
}

sk_sp<SkTypeface> OsmAnd::SkiaUtilities::createTypefaceFromData(const QByteArray& data)
{
    const auto skData = SkData::MakeWithCopy(data.constData(), data.length());
    if (!skData)
    {
        return nullptr;
    }

    return SkTypeface::MakeFromData(skData);
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
        return nullptr;
    outputBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(*outputBitmap);
    for (const auto& bitmap : constOf(bitmaps))
    {
        canvas.drawImage(bitmap->asImage(),
            (maxWidth - bitmap->width()) / 2.0f,
            (maxHeight - bitmap->height()) / 2.0f
        );
    }
    canvas.flush();

    return outputBitmap;
}
