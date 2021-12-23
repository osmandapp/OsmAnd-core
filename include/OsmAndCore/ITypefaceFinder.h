#ifndef _OSMAND_CORE_I_TYPEFACE_FINDER_H_
#define _OSMAND_CORE_I_TYPEFACE_FINDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <unordered_map>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkFontStyle.h>
#include <SkTypeface.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <hb.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ITypefaceFinder
    {
        Q_DISABLE_COPY_AND_MOVE(ITypefaceFinder);

    public:
        struct OSMAND_CORE_API Typeface Q_DECL_FINAL
        {
            Typeface(const sk_sp<SkTypeface>& skTypeface, const std::shared_ptr<hb_face_t>& hbFace);
            ~Typeface();

            sk_sp<SkTypeface> skTypeface;
            std::shared_ptr<hb_face_t> hbFace;

            std::unordered_map<uint32_t, uint32_t> replacementCodepoints;

            static std::shared_ptr<Typeface> fromData(const QByteArray& data);
        };

    private:
    protected:
        ITypefaceFinder();
    public:
        virtual ~ITypefaceFinder();

        virtual std::shared_ptr<const SWIG_CLARIFY(ITypefaceFinder, Typeface)> findTypefaceForCharacterUCS4(
            const uint32_t character,
            const SkFontStyle style = SkFontStyle()) const = 0;

    };

    SWIG_EMIT_DIRECTOR_BEGIN(ITypefaceFinder);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            std::shared_ptr<const ITypefaceFinder::Typeface>,
            findTypefaceForCharacterUCS4,
            const uint32_t character,
            SWIG_OMIT(const) SkFontStyle style);
    SWIG_EMIT_DIRECTOR_END(ITypefaceFinder);
}

#endif // !defined(_OSMAND_CORE_I_TYPEFACE_FINDER_H_)
