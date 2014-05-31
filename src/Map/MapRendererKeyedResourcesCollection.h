#ifndef _OSMAND_CORE_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "KeyedEntriesCollection.h"
#include "MapRendererBaseKeyedResource.h"
#include "MapRendererBaseResourcesCollection.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;

    // Keyed resources collection:
    class MapRendererKeyedResourcesCollection
        : public MapRendererBaseResourcesCollection
        , public KeyedEntriesCollection < const void*, MapRendererBaseKeyedResource >
    {
    private:
    protected:
        MapRendererKeyedResourcesCollection(const MapRendererResourceType& type);

        void verifyNoUploadedResourcesPresent() const;
        virtual void removeAllEntries();
    public:
        virtual ~MapRendererKeyedResourcesCollection();

        virtual int getResourcesCount() const;
        virtual void forEachResourceExecute(const ResourceActionCallback action);
        virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter);
        virtual void removeResources(const ResourceFilterCallback filter);

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_)
