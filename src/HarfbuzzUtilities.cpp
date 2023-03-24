#include "HarfbuzzUtilities.h"

std::shared_ptr<hb_face_t> OsmAnd::HarfbuzzUtilities::createFaceFromData(const QByteArray& data)
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
        hb_blob_destroy);
    if (!hbBlob)
    {
        return nullptr;
    }

    const auto pHbFace = hb_face_create(hbBlob.get(), 0);
    if (pHbFace == hb_face_get_empty())
    {
        return nullptr;
    }

    return std::shared_ptr<hb_face_t>(pHbFace, hb_face_destroy);
}

std::shared_ptr<hb_face_t> OsmAnd::HarfbuzzUtilities::createFaceFromFile(const char* filePath)
{
    const auto hbBlob = std::shared_ptr<hb_blob_t>(
        hb_blob_create_from_file_or_fail(filePath),
        hb_blob_destroy);

    if (!hbBlob)
        return nullptr;

    const auto pHbFace = hb_face_create(hbBlob.get(), 0);

    if (pHbFace == hb_face_get_empty())
        return nullptr;

    return std::shared_ptr<hb_face_t>(pHbFace, hb_face_destroy);
}
