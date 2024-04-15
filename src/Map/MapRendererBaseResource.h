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
#include "MapRendererState.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    class MapRendererBaseResource : public std::enable_shared_from_this<MapRendererBaseResource>
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererBaseResource);

    public:
        typedef std::function<void()> CancelResourceRequestSignature;
        void markAsFreshInGPU();

    private:
        volatile bool _isJunk;
        volatile bool _isOld;
        volatile bool _isOldInGPU;
        volatile bool _leaveQuietly;
    protected:
        MapRendererBaseResource(MapRendererResourcesManager* owner, MapRendererResourceType type);

        CancelResourceRequestSignature _cancelRequestCallback;

        std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> _retainableCacheMetadata;

        void markAsJunk();
        void markAsOld();
        void markAsFresh();
        void shouldLeaveQuietly();

        virtual bool updatesPresent();
        virtual bool checkForUpdatesAndApply(
            const MapState& mapState, IMapRenderer& mapRenderer);

        typedef std::function<void(const bool requestSucceeded, const bool dataAvailable)> ObtainDataAsyncCallback;
        virtual bool supportsObtainDataAsync() const = 0;
        virtual bool obtainData(bool& dataAvailable, const std::shared_ptr<const IQueryController>& queryController) = 0;
        virtual void obtainDataAsync(ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController,
            const bool cacheOnly = false) = 0;

        virtual bool uploadToGPU() = 0;
        virtual void unloadFromGPU() = 0;
        virtual void lostDataInGPU() = 0;
        virtual void releaseData() = 0;

        virtual bool supportsResourcesRenew();
        virtual void requestResourcesRenew();
        virtual void prepareResourcesRenew();
        virtual void finishResourcesRenewing();

        virtual void removeSelfFromCollection() = 0;
    public:
        virtual ~MapRendererBaseResource();

        MapRendererResourcesManager* const resourcesManager;
        const MapRendererResourceType type;

        const volatile bool& isJunk;
        const volatile bool& isOld;
        const volatile bool& isOldInGPU;
        const volatile bool& leaveQuietly;

        virtual bool isRenewing();

        virtual MapRendererResourceState getState() const = 0;
        virtual void setState(MapRendererResourceState newState) = 0;
        virtual bool setStateIf(MapRendererResourceState testState, MapRendererResourceState newState) = 0;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_)
