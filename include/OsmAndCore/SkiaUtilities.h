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
        static sk_sp<SkImage> createImageFromFile(
            const QFileInfo& fileInfo);

        static sk_sp<SkImage> createImageFromData(
            const QByteArray& data);
        
        static sk_sp<SkImage> createSkImageARGB888With(
            const QByteArray& byteArray,
            int width,
            int height);

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
