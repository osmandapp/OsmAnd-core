#ifndef _OSMAND_CORE_I_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_
#define _OSMAND_CORE_I_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "IMapKeyedDataProvider.h"

namespace OsmAnd
{
    class MapRendererBaseKeyedResource;

    class IMapRendererKeyedResourcesCollection
    {
    public:
        typedef IMapKeyedDataProvider::Key Key;

    private:
    protected:
        IMapRendererKeyedResourcesCollection();
    public:
        virtual ~IMapRendererKeyedResourcesCollection();

        virtual QList<Key> getKeys() const = 0;

        virtual bool obtainResource(
            const Key key,
            std::shared_ptr<MapRendererBaseKeyedResource>& outResource) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_KEYED_RESOURCES_COLLECTION_H_)
