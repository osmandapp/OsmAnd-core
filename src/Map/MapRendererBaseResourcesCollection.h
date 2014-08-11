#ifndef _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_MAP_RENDERER_BASE_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "IMapRendererResourcesCollection.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;
    class MapRendererBaseResource;

    class MapRendererBaseResourcesCollection : public IMapRendererResourcesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererBaseResourcesCollection);

    private:
    protected:
        MapRendererBaseResourcesCollection(const MapRendererResourceType type);
    public:
        virtual ~MapRendererBaseResourcesCollection();

        const MapRendererResourceType type;

        virtual MapRendererResourceType getType() const;

        virtual void removeResources(const ResourceFilterCallback filter) = 0;

        virtual bool updateCollectionSnapshot() const = 0;
        virtual std::shared_ptr<const IMapRendererResourcesCollection> getCollectionSnapshot() const = 0;
        virtual std::shared_ptr<IMapRendererResourcesCollection> getCollectionSnapshot() = 0;

    friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCES_COLLECTION_H_)
