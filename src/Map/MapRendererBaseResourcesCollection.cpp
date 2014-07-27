#include "MapRendererBaseResourcesCollection.h"

OsmAnd::MapRendererBaseResourcesCollection::MapRendererBaseResourcesCollection(const MapRendererResourceType type_)
    : type(type_)
{
}

OsmAnd::MapRendererBaseResourcesCollection::~MapRendererBaseResourcesCollection()
{
}

OsmAnd::MapRendererResourceType OsmAnd::MapRendererBaseResourcesCollection::getType() const
{
    return type;
}
