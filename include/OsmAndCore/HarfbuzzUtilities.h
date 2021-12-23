#ifndef _OSMAND_CORE_HARFBUZZ_UTILITIES_H_
#define _OSMAND_CORE_HARFBUZZ_UTILITIES_H_

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <hb.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/stdlib_common.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API HarfbuzzUtilities Q_DECL_FINAL
    {
        HarfbuzzUtilities() = delete;
        ~HarfbuzzUtilities() = delete;

        static std::shared_ptr<hb_face_t> createFaceFromData(const QByteArray& data);
    };
}

#endif // !defined(_OSMAND_CORE_HARFBUZZ_UTILITIES_H_)
