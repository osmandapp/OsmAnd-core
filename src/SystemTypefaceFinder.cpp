#include "SystemTypefaceFinder.h"

#include <QByteArray>

#include <hb.h>

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include "restore_internal_warnings.h"
#include "HarfbuzzUtilities.h"

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
    {
        return nullptr;
    }

    const auto skTypeface = pTypeface->makeClone({});
    if (!skTypeface)
    {
        return nullptr;
    }

    int ttcIndex = 0;
    const auto skTypefaceStream = skTypeface->openStream(&ttcIndex);
    if (!skTypefaceStream || !skTypefaceStream->hasLength())
    {
        return nullptr;
    }

    const auto dataLength = skTypefaceStream->getLength();
    const auto pData = new char[dataLength];
    const auto actualLength = skTypefaceStream->read(pData, dataLength);
    if (dataLength != actualLength)
    {
        return nullptr;
    }

    return OsmAnd::ITypefaceFinder::Typeface::fromData(QByteArray(pData, dataLength));
}
