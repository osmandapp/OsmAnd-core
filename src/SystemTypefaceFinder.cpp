#include "SystemTypefaceFinder.h"

#include <hb-ot.h>

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include "restore_internal_warnings.h"

OsmAnd::SystemTypefaceFinder::SystemTypefaceFinder(
    const sk_sp<SkFontMgr> fontManager_ /*= SkTypefaceMgr::RefDefault()*/)
    : fontManager(fontManager_)
{
}

OsmAnd::SystemTypefaceFinder::~SystemTypefaceFinder()
{
}

std::shared_ptr<const OsmAnd::ITypefaceFinder::Typeface> OsmAnd::SystemTypefaceFinder::findTypefaceForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    const auto pTypeface = fontManager->matchFamilyStyleCharacter(nullptr, style, nullptr, 0, character);
    if (!pTypeface)
        return nullptr;

    const auto skTypeface = pTypeface->makeClone({});
    if (!skTypeface)
        return nullptr;

    int ttcIndex = 0;
    const auto skTypefaceStream = skTypeface->openStream(&ttcIndex);
    if (!skTypefaceStream || !skTypefaceStream->hasLength())
        return nullptr;

    const auto dataLength = skTypefaceStream->getLength();
    const auto pData = new uint8_t[dataLength];

    const auto hbBlob = std::shared_ptr<hb_blob_t>(
        hb_blob_create_or_fail(
            reinterpret_cast<const char*>(pData),
            dataLength,
            HB_MEMORY_MODE_READONLY,
            pData,
            [](void* pUserData) { delete reinterpret_cast<uint8_t*>(pUserData); }),
        [](auto p) { hb_blob_destroy(p); });
    if (!hbBlob)
        return nullptr;

    const auto pHbTypeface = hb_face_create(hbBlob.get(), 0);
    if (!pHbTypeface || pHbTypeface == hb_face_get_empty())
        return nullptr;


    //here calculating replacement symbols
    std::set<uint32_t> delCodePoints;
    uint32_t repCodePoint;
    hb_font_t *hb_font = hb_font_create(pHbTypeface);
    hb_ot_font_set_funcs(hb_font);
    hb_buffer_t *hb_buffer = hb_buffer_create();
    hb_buffer_add_utf8(hb_buffer, ITypefaceFinder::Typeface::sRepChars, -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);
    hb_shape(hb_font, hb_buffer, NULL, 0);
    unsigned int length = hb_buffer_get_length(hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    for (int i = 0; i < length; i++)
    {
        if (i == 0)
        {
            repCodePoint = info[i].codepoint;
        } else if (i == 1) {
            continue;
        } else {
            delCodePoints.insert(info[i].codepoint);
        }
    }
    hb_buffer_destroy(hb_buffer);
    hb_font_destroy(hb_font);

    return std::make_shared<Typeface>(skTypeface,
                                      pHbTypeface,
                                      std::move(delCodePoints),
                                      repCodePoint);
}
