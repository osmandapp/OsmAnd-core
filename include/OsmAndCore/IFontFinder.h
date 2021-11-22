#ifndef _OSMAND_CORE_I_FONT_FINDER_H_
#define _OSMAND_CORE_I_FONT_FINDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <SkRefCnt.h>
#include <SkFontStyle.h>
#include <SkTypeface.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IFontFinder
    {
        Q_DISABLE_COPY_AND_MOVE(IFontFinder);

    private:
    protected:
        IFontFinder();
    public:
        virtual ~IFontFinder();

        virtual sk_sp<SkTypeface> findFontForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const = 0;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(IFontFinder);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            sk_sp<SkTypeface>,
            findFontForCharacterUCS4,
            const uint32_t character,
            SWIG_OMIT(const) SkFontStyle style);
    SWIG_EMIT_DIRECTOR_END(IFontFinder);
}

#endif // !defined(_OSMAND_CORE_I_FONT_FINDER_H_)
