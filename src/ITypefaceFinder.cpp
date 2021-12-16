#include "ITypefaceFinder.h"

#include <utility>

#include <SkTypeface.h>
#include <hb-ot.h>

#include <OsmAndCore.h>

/*static */const char OsmAnd::ITypefaceFinder::Typeface::sRepChars[] = "\xE2\x80\x8B\x41\xE2\x80\x8C";//just add code to the end with \x41 divider

OsmAnd::ITypefaceFinder::ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::~ITypefaceFinder()
{
}

OsmAnd::ITypefaceFinder::Typeface::Typeface(const sk_sp<SkTypeface>& skTypeface_,
                                            TFontPtr hbFont_,
                                            std::set<uint32_t> delCodePoints_,
                                            uint32_t repCodePoint_)
    : skTypeface(skTypeface_)
    , hbFont(std::move(hbFont_))
    , delCodePoints(delCodePoints_)
    , repCodePoint(repCodePoint_)
{
}

OsmAnd::ITypefaceFinder::Typeface::~Typeface()
{
}

/*static*/
 std::shared_ptr<OsmAnd::ITypefaceFinder::Typeface> OsmAnd::ITypefaceFinder::constructTypeface(
            const sk_sp<SkTypeface>& skTypeface,
            const std::shared_ptr<hb_blob_t>& hbBlob)
{
    if (nullptr == skTypeface || nullptr == hbBlob)
    {
        return nullptr;
    }

    const auto pHbTypeface = hb_face_create(hbBlob.get(), 0);
    if (!pHbTypeface || pHbTypeface == hb_face_get_empty())
        return nullptr;

    //here calculating replacement symbols
    std::set<uint32_t> delCodePoints;
    uint32_t repCodePoint;
    auto hb_font = TFontPtr(hb_font_create(pHbTypeface),
                            [](auto* p) { hb_font_destroy(p); });
    hb_ot_font_set_funcs(hb_font.get());
    hb_buffer_t *hb_buffer = hb_buffer_create();
    hb_buffer_add_utf8(hb_buffer, ITypefaceFinder::Typeface::sRepChars, -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);
    hb_shape(hb_font.get(), hb_buffer, NULL, 0);
    unsigned int length = hb_buffer_get_length(hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);

    repCodePoint = info[0].codepoint;
    for (size_t i = 2; i < length; ++i)
    {
        delCodePoints.insert(info[i].codepoint);
    }
    hb_buffer_destroy(hb_buffer);
    hb_face_destroy(pHbTypeface);


    return std::make_shared<Typeface>(skTypeface,
                                      std::move(hb_font),
                                      std::move(delCodePoints),
                                      repCodePoint);
}
