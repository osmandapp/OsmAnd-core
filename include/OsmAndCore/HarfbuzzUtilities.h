#ifndef _OSMAND_CORE_HARFBUZZ_UTILITIES_H_
#define _OSMAND_CORE_HARFBUZZ_UTILITIES_H_

#include <hb.h>

#include <OsmAndCore.h>
#include <OsmAndCore/stdlib_common.h>

namespace OsmAnd
{
    using THbFontPtr = std::shared_ptr<hb_font_t>;

    struct OSMAND_CORE_API HarfbuzzUtilities Q_DECL_FINAL
    {
        static constexpr float kHarfbuzzFontScale = 64.0;
        HarfbuzzUtilities() = delete;
        ~HarfbuzzUtilities() = delete;

        static THbFontPtr createHbFontFromData(const QByteArray& data);

    };
}

#endif // !defined(_OSMAND_CORE_HARFBUZZ_UTILITIES_H_)
