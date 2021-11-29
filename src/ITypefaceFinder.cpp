#include "ITypefaceFinder.h"

#include <utility>

#include <SkTypeface.h>

#include <OsmAndCore.h>

#include "Logging.h"
#include "SkiaUtilities.h"

static constexpr char kRepChars[] = "\xE2\x80\x8B\x41\xE2\x80\x8C";//just add code to the end with \x41 divider

OsmAnd::ITypefaceFinder::ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::~ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::Typeface::Typeface(const sk_sp<SkTypeface>& skTypeface_,
                                            THbFontPtr hbFont_,
                                            std::set<uint32_t> delCodePoints_,
                                            uint32_t repCodePoint_)
    : skTypeface(skTypeface_)
    , hbFont(std::move(hbFont_))
    , delCodePoints(delCodePoints_)
    , repCodePoint(repCodePoint_)
{
}

OsmAnd::ITypefaceFinder::Typeface::Typeface(const sk_sp<SkTypeface>& skTypeface_,
                                            THbFontPtr hbFont_)
    : skTypeface(skTypeface_)
    , hbFont(std::move(hbFont_))
{
    assert(nullptr != skTypeface);
    assert(nullptr != hbFont);

    /* Create hb-buffer and populate. */
    std::shared_ptr<hb_buffer_t> hb_buffer_ptr(hb_buffer_create(), [](auto* p) { hb_buffer_destroy(p); });
    if (!hb_buffer_allocation_successful(hb_buffer_ptr.get()))
    {
        return;
    }

    hb_buffer_add_utf8(hb_buffer_ptr.get(), kRepChars, -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer_ptr.get());
    hb_shape(hbFont.get(), hb_buffer_ptr.get(), NULL, 0);
    unsigned int length = hb_buffer_get_length(hb_buffer_ptr.get());
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(hb_buffer_ptr.get(), NULL);

    repCodePoint = info[0].codepoint;
    for (size_t i = 2; i < length; ++i)
    {
        delCodePoints.insert(info[i].codepoint);
    }
}

OsmAnd::ITypefaceFinder::Typeface::~Typeface()
{
}


/*static*/
 std::shared_ptr<OsmAnd::ITypefaceFinder::Typeface> OsmAnd::ITypefaceFinder::Typeface::fromData(QByteArray data)
{
    if (data.isEmpty())
    {
        return nullptr;
    }

    const auto skTypeface = SkiaUtilities::createTypefaceFromData(data);
    if (nullptr == skTypeface)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create SkTypeface from data");
        return nullptr;
    }

    auto hbFont = HarfbuzzUtilities::createHbFontFromData(data);
    if (nullptr == hbFont)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create HbFont from data");
        return nullptr;
    }

    return std::make_shared<Typeface>(skTypeface, std::move(hbFont));
}
