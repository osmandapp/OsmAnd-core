#include "HarfbuzzUtilities.h"

#include <hb-ft.h>

namespace OsmAnd
{
    /*static*/
    THbFontPtr HarfbuzzUtilities::createHbFontFromData(const QByteArray& data)
    {
        if (data.isEmpty())
        {
            return nullptr;
        }

        const auto hbBlob = std::shared_ptr<hb_blob_t>(
            hb_blob_create_or_fail(
                data.constData(),
                data.length(),
                HB_MEMORY_MODE_READONLY,
                new QByteArray(data),
                [](void* pUserData) { delete reinterpret_cast<QByteArray*>(pUserData); }),
            [](auto p) { hb_blob_destroy(p); });
        if (nullptr == hbBlob)
        {
            return nullptr;
        }

        const auto pHbTypeface = hb_face_create(hbBlob.get(), 0);
        if (nullptr == pHbTypeface)
        {
            return nullptr;
        }

        //here calculating replacement symbols
        return THbFontPtr(hb_font_create(pHbTypeface),
                        [pHbTypeface](auto* p) {
                            hb_font_destroy(p);
                            hb_face_destroy(pHbTypeface);
                        });
    }
}  // namespace OsmAnd
