#ifndef _OSMAND_CORE_SKIA_UTILITIES_H_
#define _OSMAND_CORE_SKIA_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QFileInfo>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <SkTypeface.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API SkiaUtilities Q_DECL_FINAL
    {
        static sk_sp<SkImage> getEmptyImage(
            int width,
            int height);

        static sk_sp<SkImage> getEmptyImage();

        static sk_sp<SkImage> createImageFromFile(
            const QFileInfo& fileInfo);

        static sk_sp<SkImage> createImageFromData(
            const QByteArray& data);

        static sk_sp<SkImage> createImageFromVectorData(
            const QByteArray& data,
            const float scale);
        static sk_sp<SkImage> createImageFromVectorData(
            const QByteArray& data,
            const float width,
            const float height);

        static sk_sp<SkImage> createSkImageARGB888With(
            const QByteArray& byteArray,
            int width,
            int height,
            SkAlphaType alphaType = SkAlphaType::kPremul_SkAlphaType);

        static sk_sp<SkImage> getUpperLeft(
            const sk_sp<const SkImage>& original);

        static sk_sp<SkImage> getUpperRight(
            const sk_sp<const SkImage>& original);

        static sk_sp<SkImage> getLowerLeft(
            const sk_sp<const SkImage>& original);

        static sk_sp<SkImage> getLowerRight(
            const sk_sp<const SkImage>& original);

        static sk_sp<SkImage> scaleImage(
            const sk_sp<const SkImage>& original,
            float xScale,
            float yScale);

        static sk_sp<SkImage> offsetImage(
            const sk_sp<const SkImage>& original,
            float xOffset,
            float yOffset);

        static sk_sp<SkImage> createTileImage(
            const sk_sp<const SkImage>& first,
            const sk_sp<const SkImage>& second,
            float yOffset);

        static sk_sp<SkImage> cropImage(
            const sk_sp<const SkImage>& image,
            const uint32_t cutPart);

        static sk_sp<SkImage> stackImages(
            const sk_sp<const SkImage>& top,
            const sk_sp<const SkImage>& bottom,
            const sk_sp<const SkImage>& extraBottom,
            const SkAlphaType alphaType);

        static sk_sp<SkImage> joinImages(
            const sk_sp<const SkImage>& left,
            const sk_sp<const SkImage>& right);

        static sk_sp<SkImage> createImageFromRawData(const QByteArray& byteArray);

        static QByteArray getRawDataFromImage(const sk_sp<const SkImage>& sourceImage);

        static sk_sp<SkTypeface> createTypefaceFromData(
            const QByteArray& data);

        static sk_sp<SkImage> mergeImages(
            const QList<sk_sp<const SkImage>>& images);

        static sk_sp<SkImage> mergeImages(
            const QList<sk_sp<const SkImage>>& images,
            const QList<float>& alphas);

    private:
        SkiaUtilities();
        ~SkiaUtilities();
    };
}

#endif // !defined(_OSMAND_CORE_SKIA_UTILITIES_H_)
