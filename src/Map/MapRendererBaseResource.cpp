#include "MapRendererBaseResource.h"

OsmAnd::MapRendererBaseResource::MapRendererBaseResource(
    MapRendererResourcesManager* const owner_,
    const MapRendererResourceType type_)
    : _isJunk(false)
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

bool OsmAnd::MapRendererBaseResource::updatesPresent()
{
    return false;
}

bool OsmAnd::MapRendererBaseResource::checkForUpdatesAndApply(const MapState& mapState)
{
    return false;
}

bool OsmAnd::MapRendererBaseResource::supportsResourcesRenew()
{
    return false;
}

void OsmAnd::MapRendererBaseResource::requestResourcesRenew()
{
}

bool OsmAnd::MapRendererBaseResource::isRenewing()
{
    return false;
}

void OsmAnd::MapRendererBaseResource::prepareResourcesRenew()
{
}

void OsmAnd::MapRendererBaseResource::finishResourcesRenewing()
{
}
