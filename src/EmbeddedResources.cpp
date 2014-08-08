#include "EmbeddedResources.h"
#include "EmbeddedResources_private.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkImageDecoder.h>
#include "restore_internal_warnings.h"

#include "Logging.h"

OsmAnd::EmbeddedResources::EmbeddedResources()
{
}

OsmAnd::EmbeddedResources::~EmbeddedResources()
{
}

QByteArray OsmAnd::EmbeddedResources::decompressResource(const QString& name, bool* ok_ /*= nullptr*/)
{
    bool ok = false;
    const auto compressedData = getRawResource(name, &ok);
    if (!ok)
    {
        if (ok_)
            *ok_ = false;
        return QByteArray();
    }

    const auto decompressedData = qUncompress(compressedData);
    if (ok_)
        *ok_ = !decompressedData.isNull();
    return decompressedData;
}

QByteArray OsmAnd::EmbeddedResources::getRawResource(const QString& name, bool* ok /*= nullptr*/)
{
    for (auto idx = 0u; idx < __bundled_resources_count; idx++)
    {
        if (__bundled_resources[idx].name != name)
            continue;

        if (ok)
            *ok = true;
        return QByteArray(reinterpret_cast<const char*>(__bundled_resources[idx].data), __bundled_resources[idx].size);
    }

    LogPrintf(LogSeverityLevel::Error, "Embedded resource '%s' was not found", qPrintable(name));
    if (ok)
        *ok = false;
    return QByteArray();
}

std::shared_ptr<SkBitmap> OsmAnd::EmbeddedResources::getBitmapResource(const QString& name, bool* ok_ /*= nullptr*/)
{
    bool ok = false;
    const auto data = decompressResource(name, &ok);
    if (!ok)
    {
        if (ok_)
            *ok_ = false;
        return nullptr;
    }

    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    SkMemoryStream dataStream(data.constData(), data.length(), false);
    if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
    {
        if (ok_)
            *ok_ = false;
        return nullptr;
    }

    if (ok_)
        *ok_ = true;
    return bitmap;
}

bool OsmAnd::EmbeddedResources::containsResource(const QString& name)
{
    for (auto idx = 0u; idx < __bundled_resources_count; idx++)
    {
        if (__bundled_resources[idx].name != name)
            continue;

        return true;
    }
    return false;
}
