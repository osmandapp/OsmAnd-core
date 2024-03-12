#include "MapRendererBaseResource.h"

OsmAnd::MapRendererBaseResource::MapRendererBaseResource(
    MapRendererResourcesManager* const owner_,
    const MapRendererResourceType type_)
    : _isJunk(false)
    , _isOld(false)
    , _isOldInGPU(false)
    , resourcesManager(owner_)
    , type(type_)
    , isJunk(_isJunk)
    , isOld(_isOld)
    , isOldInGPU(_isOldInGPU)
{
}

OsmAnd::MapRendererBaseResource::~MapRendererBaseResource()
{
}

void OsmAnd::MapRendererBaseResource::markAsJunk()
{
    _isJunk = true;
}

void OsmAnd::MapRendererBaseResource::markAsOld()
{
    _isOld = true;
}

void OsmAnd::MapRendererBaseResource::markAsFresh()
{
    _isOld = false;
    _isOldInGPU = true;
}

void OsmAnd::MapRendererBaseResource::markAsFreshInGPU()
{
    _isOldInGPU = false;
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
