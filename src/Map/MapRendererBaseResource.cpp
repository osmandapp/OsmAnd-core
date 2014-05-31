#include "MapRendererBaseResource.h"

OsmAnd::MapRendererBaseResource::MapRendererBaseResource(MapRendererResourcesManager* owner_, const MapRendererResourceType type_)
    : _isJunk(false)
    , _requestTask(nullptr)
    , owner(owner_)
    , type(type_)
    , isJunk(_isJunk)
{
}

OsmAnd::MapRendererBaseResource::~MapRendererBaseResource()
{
}

void OsmAnd::MapRendererBaseResource::markAsJunk()
{
    _isJunk = true;
}
