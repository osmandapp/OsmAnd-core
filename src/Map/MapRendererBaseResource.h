#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "IQueryController.h"
#include "IMapDataProvider.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererBaseResource : public std::enable_shared_from_this<MapRendererBaseResource>
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererBaseResource)

    public:
        typedef std::function<void()> CancelResourceRequestSignature;

    private:
        bool _isJunk;
    protected:
        MapRendererBaseResource(
            MapRendererResourcesManager* const owner,
            const MapRendererResourceType type);

        CancelResourceRequestSignature _cancelRequestCallback;

        std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> _retainableCacheMetadata;

        void markAsJunk();

        virtual bool updatesPresent();
        virtual bool checkForUpdatesAndApply();
        
        typedef std::function < void(const bool requestSucceeded, const bool dataAvailable) > ObtainDataAsyncCallback;
        virtual bool supportsObtainDataAsync() const = 0;
        virtual bool obtainData(
            bool& dataAvailable,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) = 0;
        virtual void obtainDataAsync(
            const ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) = 0;

        virtual bool uploadToGPU() = 0;
        virtual void unloadFromGPU() = 0;
        virtual void lostDataInGPU() = 0;
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

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_)
