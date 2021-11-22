#ifndef _OSMAND_CORE_SKIA_UTILITIES_H_
#define _OSMAND_CORE_SKIA_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QByteArray>
#include <SkRefCnt.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

class SkBitmap;
class SkTypeface;

namespace OsmAnd
{
    struct OSMAND_CORE_API SkiaUtilities Q_DECL_FINAL
    {
        static std::shared_ptr<SkBitmap> scaleBitmap(
            const std::shared_ptr<const SkBitmap>& original,
            const float xScale,
            const float yScale);

        static std::shared_ptr<SkBitmap> offsetBitmap(
            const std::shared_ptr<const SkBitmap>& original,
            const float xOffset,
            const float yOffset);

        static std::shared_ptr<SkBitmap> createTileBitmap(
            const std::shared_ptr<const SkBitmap>& firstBitmap,
            const std::shared_ptr<const SkBitmap>& secondBitmap,
            const float yOffset);

        static sk_sp<SkTypeface> createTypefaceFromData(
            const QByteArray& data);

        static std::shared_ptr<SkBitmap> mergeBitmaps(
            const QList< std::shared_ptr<const SkBitmap> >& bitmaps);

    private:
        SkiaUtilities();
        ~SkiaUtilities();
    };
}

#endif // !defined(_OSMAND_CORE_SKIA_UTILITIES_H_)
