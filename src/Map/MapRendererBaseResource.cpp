#include "MapRendererBaseResource.h"

OsmAnd::MapRendererBaseResource::MapRendererBaseResource(
    MapRendererResourcesManager* const owner_,
    const MapRendererResourceType type_,
    const IMapDataProvider::SourceType sourceType_)
    : _isJunk(false)
    , _requestTask(nullptr)
    , resourcesManager(owner_)
    , type(type_)
    , sourceType(sourceType_)
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

bool OsmAnd::MapRendererBaseResource::updatesPresent()
{
    return false;
}

bool OsmAnd::MapRendererBaseResource::checkForUpdatesAndApply()
{
    return false;
}
