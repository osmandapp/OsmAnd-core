#include "EmbeddedResources.h"
#include "EmbeddedResources_private.h"

#include "Logging.h"

OsmAnd::EmbeddedResources::EmbeddedResources()
{
}

OsmAnd::EmbeddedResources::~EmbeddedResources()
{
}

QByteArray OsmAnd::EmbeddedResources::decompressResource( const QString& id, bool* ok_ /*= nullptr*/ )
{
    bool ok = false;
    const auto compressedData = getRawResource(id, &ok);
    if(!ok)
    {
        if(ok_)
            *ok_ = false;
        return QByteArray();
    }

    return qUncompress(compressedData);
}

QByteArray OsmAnd::EmbeddedResources::getRawResource( const QString& id, bool* ok /*= nullptr*/ )
{
    for(auto idx = 0u; idx < __bundled_resources_count; idx++)
    {
        if(__bundled_resources[idx].id != id)
            continue;

        if(ok)
            *ok = true;
        return QByteArray(reinterpret_cast<const char*>(__bundled_resources[idx].data), __bundled_resources[idx].size);
    }

    LogPrintf(LogSeverityLevel::Error, "Embedded resource '%s' was not found", qPrintable(id));
    if(ok)
        *ok = false;
    return QByteArray();
}

bool OsmAnd::EmbeddedResources::containsResource( const QString& id )
{
    for(auto idx = 0u; idx < __bundled_resources_count; idx++)
    {
        if(__bundled_resources[idx].id != id)
            continue;

        return true;
    }
    return false;
}
