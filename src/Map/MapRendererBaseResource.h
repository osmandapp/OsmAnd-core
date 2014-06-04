#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "Concurrent.h"
#include "IQueryController.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    // Base resource
    class MapRendererBaseResource : public std::enable_shared_from_this<MapRendererBaseResource>
    {
    private:
        bool _isJunk;
    protected:
        MapRendererBaseResource(MapRendererResourcesManager* owner, const MapRendererResourceType type);

        Concurrent::Task* _requestTask;

        void markAsJunk();

        virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController = nullptr) = 0;
        virtual bool uploadToGPU() = 0;
        virtual void unloadFromGPU() = 0;
        virtual void releaseData() = 0;

        virtual void removeSelfFromCollection() = 0;
    public:
        virtual ~MapRendererBaseResource();

        MapRendererResourcesManager* const resourcesManager;
        const MapRendererResourceType type;

        const bool& isJunk;

        virtual MapRendererResourceState getState() const = 0;
        virtual void setState(const MapRendererResourceState newState) = 0;
        virtual bool setStateIf(const MapRendererResourceState testState, const MapRendererResourceState newState) = 0;

        virtual bool prepareForUse();

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_)
