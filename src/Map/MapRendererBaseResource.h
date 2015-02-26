#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "Concurrent.h"
#include "IQueryController.h"
#include "IMapDataProvider.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererBaseResource : public std::enable_shared_from_this<MapRendererBaseResource>
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererBaseResource);

    private:
        bool _isJunk;
    protected:
        MapRendererBaseResource(
            MapRendererResourcesManager* const owner,
            const MapRendererResourceType type,
            const IMapDataProvider::SourceType sourceType);

        Concurrent::Task* _requestTask;

        std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> _retainableCacheMetadata;

        void markAsJunk();

        virtual bool updatesPresent();
        virtual bool checkForUpdatesAndApply();
        
        virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController = nullptr) = 0;
        virtual bool uploadToGPU() = 0;
        virtual void unloadFromGPU() = 0;
        virtual void lostDataInGPU() = 0;
        virtual void releaseData() = 0;

        virtual void removeSelfFromCollection() = 0;
    public:
        virtual ~MapRendererBaseResource();

        MapRendererResourcesManager* const resourcesManager;
        const MapRendererResourceType type;
        const IMapDataProvider::SourceType sourceType;

        const bool& isJunk;

        virtual MapRendererResourceState getState() const = 0;
        virtual void setState(const MapRendererResourceState newState) = 0;
        virtual bool setStateIf(const MapRendererResourceState testState, const MapRendererResourceState newState) = 0;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_)
