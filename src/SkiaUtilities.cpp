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
    scaledBitmap->setConfig(original->config(), scaledWidth, scaledHeight);
    scaledBitmap->allocPixels();
    scaledBitmap->eraseColor(SK_ColorTRANSPARENT);

    SkCanvas canvas(*scaledBitmap);
    canvas.scale(xScale, yScale);
    canvas.drawBitmap(*original, 0, 0, NULL);
    canvas.flush();

    return scaledBitmap;
}

#if defined(OSMAND_TARGET_OS_android)
#include <SkString.h>
#include <SkTypefaceCache.h>
#include <SkFontDescriptor.h>
#include <SkFontHost_FreeType_common.h>

// Defined in SkFontHost_FreeType.cpp
bool find_name_and_attributes(
    SkStream* stream,
    SkString* name,
    SkTypeface::Style* style,
    bool* isFixedWidth);

namespace OsmAnd
{
    // Based on SkFontHost_linux.cpp
    class SkTypeface_StreamOnAndroidWorkaround : public SkTypeface_FreeType
    {
    public:
        SkTypeface_StreamOnAndroidWorkaround(Style style, SkStream* stream, bool isFixedPitch, const SkString familyName)
            : SkTypeface_FreeType(style, SkTypefaceCache::NewFontID(), isFixedPitch)
            , _stream(SkRef(stream))
            , _familyName(familyName)
        {
        }

        virtual ~SkTypeface_StreamOnAndroidWorkaround()
        {
        }

    protected:
        virtual void onGetFontDescriptor(SkFontDescriptor* desc, bool* isLocal) const SK_OVERRIDE
        {
            desc->setFamilyName(_familyName.c_str());
            desc->setFontFileName(nullptr);
            *isLocal = true;
        }

            virtual SkStream* onOpenStream(int* ttcIndex) const SK_OVERRIDE
        {
            *ttcIndex = 0;
            return _stream->duplicate();
        }

    private:
        SkAutoTUnref<SkStream> _stream;
        SkString _familyName;
    };
}
#endif // defined(OSMAND_TARGET_OS_android)

SkTypeface* OsmAnd::SkiaUtilities::createTypefaceFromData(const QByteArray& data)
{
    SkTypeface* typeface = nullptr;

    const auto fontDataStream = new SkMemoryStream(data.constData(), data.length(), true);
#if defined(OSMAND_TARGET_OS_android)
    //WORKAROUND: Right now SKIA on Android lost ability to load custom fonts, since it doesn't provide SkFontMgr.
    //WORKAROUND: But it still loads system fonts internally, so just write direct code

    SkTypeface::Style typefaceStyle;
    bool typefaceIsFixedWidth = false;
    SkString typefaceName;
    const auto isValidTypeface = find_name_and_attributes(fontDataStream, &typefaceName, &typefaceStyle, &typefaceIsFixedWidth);
    if (isValidTypeface)
        typeface = SkNEW_ARGS(SkTypeface_StreamOnAndroidWorkaround, (typefaceStyle, fontDataStream, typefaceIsFixedWidth, typefaceName));
#else // if !defined(OSMAND_TARGET_OS_android)
    typeface = SkTypeface::CreateFromStream(fontDataStream);
#endif
    fontDataStream->unref();
    
    return typeface;
}
