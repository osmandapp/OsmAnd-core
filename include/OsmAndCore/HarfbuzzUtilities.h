#ifndef _OSMAND_CORE_HARFBUZZ_UTILITIES_H_
#define _OSMAND_CORE_HARFBUZZ_UTILITIES_H_

#include <hb.h>

#include <OsmAndCore.h>
#include <OsmAndCore/stdlib_common.h>

namespace OsmAnd
{
    using THbFontPtr = std::unique_ptr<hb_font_t, std::function<void (hb_font_t*)>>;
    using THbBufferPtr = std::unique_ptr<hb_buffer_t, std::function<void (hb_buffer_t*)>>;

    struct OSMAND_CORE_API HarfbuzzUtilities Q_DECL_FINAL
    {
        HarfbuzzUtilities() = delete;
        ~HarfbuzzUtilities() = delete;

        static THbFontPtr createHbFontFromData(const QByteArray& data);

    };
}

#endif // !defined(_OSMAND_CORE_HARFBUZZ_UTILITIES_H_)
