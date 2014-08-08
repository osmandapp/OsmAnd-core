#ifndef _OSMAND_CORE_I_MAP_RENDERER_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_I_MAP_RENDERER_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"

namespace OsmAnd
{
    class MapRendererBaseResource;

    class IMapRendererResourcesCollection
    {
        Q_DISABLE_COPY(IMapRendererResourcesCollection);

    public:
        typedef std::function<void (const std::shared_ptr<MapRendererBaseResource>& resource, bool& cancel)> ResourceActionCallback;
        typedef std::function<bool (const std::shared_ptr<MapRendererBaseResource>& resource, bool& cancel)> ResourceFilterCallback;

    private:
    protected:
        IMapRendererResourcesCollection();
    public:
        virtual ~IMapRendererResourcesCollection();

        virtual MapRendererResourceType getType() const = 0;

        virtual int getResourcesCount() const = 0;
        virtual void forEachResourceExecute(const ResourceActionCallback action) = 0;
        virtual void obtainResources(QList< std::shared_ptr<MapRendererBaseResource> >* outList, const ResourceFilterCallback filter) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_RESOURCES_COLLECTION_H_)
