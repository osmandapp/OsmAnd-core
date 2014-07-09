#include "MapRendererBaseResource.h"

OsmAnd::MapRendererBaseResource::MapRendererBaseResource(MapRendererResourcesManager* owner_, const MapRendererResourceType type_)
    : _isJunk(false)
    , _requestTask(nullptr)
    , resourcesManager(owner_)
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

bool OsmAnd::MapRendererBaseResource::checkForUpdates()
{
    return false;
}
