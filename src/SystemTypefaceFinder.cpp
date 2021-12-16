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

    return OsmAnd::ITypefaceFinder::constructTypeface(skTypeface, hbBlob);
}
