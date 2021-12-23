#include "ITypefaceFinder.h"

#include "Logging.h"
#include "SkiaUtilities.h"
#include "HarfbuzzUtilities.h"

OsmAnd::ITypefaceFinder::ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::~ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::Typeface::Typeface(
    const sk_sp<SkTypeface>& skTypeface_,
    const std::shared_ptr<hb_face_t>& hbFace_)
    : skTypeface(skTypeface_)
    , hbFace(hbFace_)
{
    const auto pHbBuffer = hb_buffer_create();
    if (pHbBuffer == hb_buffer_get_empty())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create hb_buffer_t");
        return;
    }
    std::shared_ptr<hb_buffer_t> hbBuffer(pHbBuffer, hb_buffer_destroy);
    if (!hb_buffer_allocation_successful(hbBuffer.get()))
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to allocate hb_buffer_t");
        return;
    }

    std::shared_ptr<hb_font_t> hbFont(
        hb_font_create(hbFace.get()),
        hb_font_destroy
    );

    hb_buffer_add_utf8(
        hbBuffer.get(),
        "\xE2\x80\x8B\x41\xE2\x80\x8C", // NOTE: UTF8 characters with \x41 divider
        -1,
        0,
        -1
    );
    hb_buffer_guess_segment_properties(hbBuffer.get());
    hb_shape(hbFont.get(), hbBuffer.get(), nullptr, 0);

    unsigned int glyphsCount = 0;
    const auto pGlyphInfos = hb_buffer_get_glyph_infos(hbBuffer.get(), &glyphsCount);
    if (glyphsCount > 0)
    {
        const auto replacementCodepoint = pGlyphInfos[0].codepoint;
        for (auto glyphIdx = 2u; glyphIdx < glyphsCount; glyphIdx++)
        {
            replacementCodepoints[pGlyphInfos[glyphIdx].codepoint] = replacementCodepoint;
        }
    }
}

OsmAnd::ITypefaceFinder::Typeface::~Typeface()
{
}

std::shared_ptr<OsmAnd::ITypefaceFinder::Typeface> OsmAnd::ITypefaceFinder::Typeface::fromData(const QByteArray& data)
{
    if (data.isEmpty())
    {
        return nullptr;
    }

    const auto skTypeface = SkiaUtilities::createTypefaceFromData(data);
    if (!skTypeface)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create SkTypeface from data");
        return nullptr;
    }

    const auto hbFace = HarfbuzzUtilities::createFaceFromData(data);
    if (!hbFace)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create hb_face_t from data");
        return nullptr;
    }

    return std::make_shared<Typeface>(skTypeface, hbFace);
}
