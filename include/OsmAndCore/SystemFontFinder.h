#ifndef _OSMAND_CORE_SYSTEM_FONT_FINDER_H_
#define _OSMAND_CORE_SYSTEM_FONT_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <SkFontMgr.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IFontFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API SystemFontFinder Q_DECL_FINAL : public IFontFinder
    {
        Q_DISABLE_COPY_AND_MOVE(SystemFontFinder);

    private:
    protected:
    public:
        SystemFontFinder(const SkFontMgr* const fontManager = SkFontMgr::RefDefaultRaw());
        virtual ~SystemFontFinder();

        const SkFontMgr* const fontManager;

        virtual SkTypeface* findFontForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_SYSTEM_FONT_FINDER_H_)
