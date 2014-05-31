#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;
    class MapRendererBaseResource;

    // Base Collection of resources
    class MapRendererBaseResourcesCollection
    {
    public:
        typedef std::function<void(const std::shared_ptr<MapRendererBaseResource>& resource, bool& cancel)> ResourceActionCallback;
        typedef std::function<bool(const std::shared_ptr<MapRendererBaseResource>& resource, bool& cancel)> ResourceFilterCallback;

    private:
    protected:
        MapRendererBaseResourcesCollection(const MapRendererResourceType& type);
    public:
        virtual ~MapRendererBaseResourcesCollection();

        const MapRendererResourceType type;

        virtual int getResourcesCount() const = 0;
        virtual void forEachResourceExecute(const ResourceActionCallback action) = 0;
        virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter) = 0;
        virtual void removeResources(const ResourceFilterCallback filter) = 0;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCES_COLLECTION_H_)
