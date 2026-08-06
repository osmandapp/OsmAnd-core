#include "ObfConstants.h"

bool OsmAnd::ObfConstants::isTagIndexedForSearchAsName(const QString& tag)
{
    if (!tag.isNull())
    {
        // search related but not direct
        if (tag.startsWith(QStringLiteral("route_name"))
            || tag == QStringLiteral("shield_stub_name"))
        {
            return false;
        }
        // some popular tags ignored as name
        if (tag.startsWith(QStringLiteral("tiger:"))
            || tag.startsWith(QStringLiteral("noname"))
            || tag.startsWith(QStringLiteral("name:etymology"))
            || tag.startsWith(QStringLiteral("artist_name"))
            || tag.startsWith(QStringLiteral("addr:"))) // housename, street name
        {
            return false;
        }
        return tag.contains(QStringLiteral("name")) || tag.contains(QStringLiteral("brand"));
    }
    return false;
}

bool OsmAnd::ObfConstants::isTagIndexedForSearchAsId(const QString& tag)
{
    if (!tag.isNull())
    {
        return tag == QStringLiteral("wikidata") || tag == QStringLiteral("route_id");
    }
    return false;
}

bool OsmAnd::ObfConstants::isTagIndexedAsSearchRelated(const QString& tag)
{
    if (!tag.isNull())
    {
        return tag == QStringLiteral("route_members_ids") || tag == QStringLiteral("route_name");
    }
    return false;
}
