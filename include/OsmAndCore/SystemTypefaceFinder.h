#ifndef _OSMAND_CORE_SYSTEM_TYPEFACE_FINDER_H_
#define _OSMAND_CORE_SYSTEM_TYPEFACE_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <SkFontMgr.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/ITypefaceFinder.h>

namespace OsmAnd
{
    class OSMAND_CORE_API SystemTypefaceFinder Q_DECL_FINAL : public ITypefaceFinder
    {
        Q_DISABLE_COPY_AND_MOVE(SystemTypefaceFinder);

    private:
    protected:
    public:
        SystemTypefaceFinder(const sk_sp<SkFontMgr> fontManager = SkFontMgr::RefDefault());
        virtual ~SystemTypefaceFinder();

        const sk_sp<SkFontMgr> fontManager;

        virtual std::shared_ptr<const Typeface> findTypefaceForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_SYSTEM_TYPEFACE_FINDER_H_)
