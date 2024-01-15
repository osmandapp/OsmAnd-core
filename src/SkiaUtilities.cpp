#include "SkiaUtilities.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkStream.h>
#include <SkSVGSVG.h>
#include <SkSVGDOM.h>
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

sk_sp<SkImage> OsmAnd::SkiaUtilities::getEmptyImage(int width, int height)
{
    SkBitmap bitmap;
    if (bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(width, height)))
    {
        bitmap.eraseColor(SK_ColorTRANSPARENT);
        return bitmap.asImage();
    }
    return nullptr;
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::createImageFromFile(const QFileInfo& fileInfo)
{
    return SkImage::MakeFromEncoded(SkData::MakeFromFileName(qPrintable(fileInfo.absoluteFilePath())));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::createImageFromData(const QByteArray& data)
{
    return SkImage::MakeFromEncoded(SkData::MakeWithProc(
        data.constData(),
        data.length(),
        [](const void* ptr, void* context) { delete reinterpret_cast<QByteArray*>(context); },
        new QByteArray(data)
    ));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::createImageFromVectorData(const QByteArray& data, const float scale)
{
    if (data.isEmpty())
        return nullptr;

    const auto stream = SkMemoryStream::MakeDirect(data.constData(), data.size());
    const auto svgDom = SkSVGDOM::MakeFromStream(*stream);

    const auto originalSize = svgDom->containerSize();
    const SkSVGTransformType transform(SkMatrix::Scale(scale, scale));
    svgDom->getRoot()->setTransform(transform);

    SkBitmap bitmap;
    const int scaledSizeX = qCeil(originalSize.width() * scale);
    const int scaledSizeY = qCeil(originalSize.height() * scale);
    if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(scaledSizeX, scaledSizeY)))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate pixels");
        return nullptr;
    }
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    
    SkCanvas canvas(bitmap);
    svgDom->render(&canvas);
    return bitmap.asImage();
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::createImageFromVectorData(const QByteArray& data, const float width, const float height)
{
    if (data.isEmpty())
        return nullptr;

    const auto stream = SkMemoryStream::MakeDirect(data.constData(), data.size());
    const auto svgDom = SkSVGDOM::MakeFromStream(*stream);

    const auto originalSize = svgDom->containerSize();
    auto scaleX = width / originalSize.width();
    auto scaleY = height / originalSize.height();
    auto scale = qMin(scaleX, scaleY);
    const SkSVGTransformType transform(SkMatrix::Scale(scale, scale));
    svgDom->getRoot()->setTransform(transform);

    SkBitmap bitmap;
    const int scaledSizeX = qCeil(originalSize.width() * scale);
    const int scaledSizeY = qCeil(originalSize.height() * scale);
    if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(scaledSizeX, scaledSizeY)))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate pixels");
        return nullptr;
    }
    bitmap.eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(bitmap);
    svgDom->render(&canvas);
    return bitmap.asImage();
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::createSkImageARGB888With(
    const QByteArray& byteArray, int width, int height, SkAlphaType alphaType /* = SkAlphaType::kPremul_SkAlphaType */)
{
    SkBitmap bitmap;
   
    if (!bitmap.tryAllocPixels(SkImageInfo::Make(
        width,
        height,
        SkColorType::kRGBA_8888_SkColorType,
        alphaType)))
    {
        return nullptr;
    }
    if (bitmap.computeByteSize() < byteArray.size())
        return nullptr;
    memcpy(bitmap.getPixels(), byteArray.data(), byteArray.size());

    bitmap.setImmutable(); // Don't copy when we create an image.
    return bitmap.asImage();
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::getUpperLeft(
    const sk_sp<const SkImage>& original)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto scaledWidth = original->width() / 2;
    const auto scaledHeight = original->height() / 2;

    if (scaledWidth <= 0 || scaledHeight <= 0)
        return nullptr;
   
    return original->makeSubset(SkIRect::MakeXYWH(0, 0, scaledWidth, scaledHeight));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::getUpperRight(
    const sk_sp<const SkImage>& original)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto scaledWidth = original->width() / 2;
    const auto scaledHeight = original->height() / 2;

    if (scaledWidth <= 0 || scaledHeight <= 0)
        return nullptr;

    return original->makeSubset(SkIRect::MakeXYWH(scaledWidth, 0, scaledWidth, scaledHeight));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::getLowerLeft(
    const sk_sp<const SkImage>& original)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto scaledWidth = original->width() / 2;
    const auto scaledHeight = original->height() / 2;

    if (scaledWidth <= 0 || scaledHeight <= 0)
        return nullptr;

    return original->makeSubset(SkIRect::MakeXYWH(0, scaledHeight, scaledWidth, scaledHeight));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::getLowerRight(
    const sk_sp<const SkImage>& original)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto scaledWidth = original->width() / 2;
    const auto scaledHeight = original->height() / 2;

    if (scaledWidth <= 0 || scaledHeight <= 0)
        return nullptr;

    return original->makeSubset(SkIRect::MakeXYWH(scaledWidth, scaledHeight, scaledWidth, scaledHeight));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::scaleImage(
    const sk_sp<const SkImage>& original,
    float xScale,
    float yScale)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto scaledWidth = qCeil(original->width() * xScale);
    const auto scaledHeight = qCeil(original->height() * yScale);

    if (scaledWidth <= 0 || scaledHeight <= 0)
        return nullptr;

    SkBitmap target;
    if (!target.tryAllocPixels(original->imageInfo().makeWH(scaledWidth, scaledHeight)))
        return nullptr;
    
    if (!original->scalePixels(target.pixmap(), {}))
        return nullptr;

    return target.asImage();
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::offsetImage(
    const sk_sp<const SkImage>& original,
    float xOffset,
    float yOffset)
{
    if (!original || original->width() <= 0 || original->height() <= 0)
        return nullptr;

    const auto newWidth = original->width() + qAbs(xOffset);
    const auto newHeight = original->height() + qAbs(yOffset);

    SkBitmap target;
    if (!target.tryAllocPixels(original->imageInfo().makeWH(newWidth, newHeight)))
        return nullptr;
    target.eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(target);
    canvas.drawImage(original.get(), xOffset > 0.0f ? xOffset : 0, yOffset > 0.0f ? yOffset : 0);
    canvas.flush();

    return target.asImage();
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::createTileImage(
    const sk_sp<const SkImage>& first,
    const sk_sp<const SkImage>& second,
    float yOffset)
{
    SkImageInfo imageInfo;
    if (first && first->width() > 0 && first->height() > 0)
        imageInfo = first->imageInfo();

    if (second && second->width() > 0 && second->height() > 0)
        imageInfo = second->imageInfo();

    if (imageInfo.isEmpty())
        return nullptr;

    SkBitmap target;
    if (!target.tryAllocPixels(imageInfo))
        return nullptr;
    target.eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(target);
    if (first)
        canvas.drawImage(first.get(), 0, -yOffset);
    if (second)
        canvas.drawImage(second.get(), 0, -yOffset + imageInfo.height());
    canvas.flush();

    return target.asImage();
}

sk_sp<SkTypeface> OsmAnd::SkiaUtilities::createTypefaceFromData(const QByteArray& data)
{
    return SkTypeface::MakeFromData(SkData::MakeWithCopy(data.constData(), data.length()));
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::mergeImages(const QList<sk_sp<const SkImage>>& images)
{
    if (images.isEmpty())
        return nullptr;

    int maxWidth = 0;
    int maxHeight = 0;
    for (const auto& image : constOf(images))
    {
        maxWidth = qMax(maxWidth, image->width());
        maxHeight = qMax(maxHeight, image->height());
    }

    if (maxWidth <= 0 || maxHeight <= 0)
        return nullptr;

    SkBitmap target;
    if (!target.tryAllocPixels(images.first()->imageInfo().makeWH(maxWidth, maxHeight)))
        return nullptr;
    target.eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(target);
    for (const auto& image : constOf(images))
    {
        canvas.drawImage(image.get(),
            (maxWidth - image->width()) / 2.0f,
            (maxHeight - image->height()) / 2.0f
        );
    }
    canvas.flush();

    return target.asImage();
}

sk_sp<SkImage> OsmAnd::SkiaUtilities::mergeImages(const QList<sk_sp<const SkImage>>& images, const QList<float>& alphas)
{
    if (images.isEmpty() || images.size() != alphas.size())
        return nullptr;

    int maxWidth = 0;
    int maxHeight = 0;
    for (const auto& image : constOf(images))
    {
        maxWidth = qMax(maxWidth, image->width());
        maxHeight = qMax(maxHeight, image->height());
    }

    if (maxWidth <= 0 || maxHeight <= 0)
        return nullptr;

    SkBitmap target;
    if (!target.tryAllocPixels(images.first()->imageInfo().makeWH(maxWidth, maxHeight)))
        return nullptr;
    target.eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(target);
    int i = 0;
    SkPaint paint1;
    SkPaint paint2;
    for (const auto& image : constOf(images))
    {
        SkSamplingOptions so;
        SkPaint paint;
        paint.setAlphaf(alphas[i++]);
        canvas.drawImage(
            image.get(),
            (maxWidth - image->width()) / 2.0f,
            (maxHeight - image->height()) / 2.0f,
            so,
            &paint
        );
    }
    canvas.flush();

    return target.asImage();
}
