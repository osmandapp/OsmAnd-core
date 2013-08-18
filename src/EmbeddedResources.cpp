#include "EmbeddedResources.h"
#include "EmbeddedResources_private.h"

OsmAnd::EmbeddedResources::EmbeddedResources()
{
}

OsmAnd::EmbeddedResources::~EmbeddedResources()
{
}

QByteArray OsmAnd::EmbeddedResources::decompressResource( const QString& id )
{
    return qUncompress(getRawResource(id));
}

QByteArray OsmAnd::EmbeddedResources::getRawResource( const QString& id )
{
    for(auto idx = 0u; idx < __bundled_resources_count; idx++)
    {
        if(__bundled_resources[idx].id != id)
            continue;

        return QByteArray(reinterpret_cast<const char*>(__bundled_resources[idx].data), __bundled_resources[idx].size);
    }
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
