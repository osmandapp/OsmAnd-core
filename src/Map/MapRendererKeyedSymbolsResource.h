#ifndef _OSMAND_CORE_MAP_RENDERER_KEYED_SYMBOLS_RESOURCE_H_
#define _OSMAND_CORE_MAP_RENDERER_KEYED_SYMBOLS_RESOURCE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "MapRendererResourceType.h"
#include "MapRendererResourceState.h"
#include "KeyedEntriesCollection.h"
#include "GPUAPI.h"
#include "MapRendererBaseKeyedResource.h"

namespace OsmAnd
{
    class MapRendererResourcesManager;
    class MapSymbolsGroup;
    class MapSymbol;

    // Keyed symbols:
    class MapRendererKeyedSymbolsResource : public MapRendererBaseKeyedResource
    {
    private:
    protected:
        MapRendererKeyedSymbolsResource(MapRendererResourcesManager* owner, const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection, const Key key);

        std::shared_ptr<const MapSymbolsGroup> _sourceData;
        QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > _resourcesInGPU;

        virtual bool obtainData(bool& dataAvailable, const IQueryController* queryController);
        virtual bool uploadToGPU();
        virtual void unloadFromGPU();

        virtual bool checkIsSafeToUnlink();
        virtual void detach();
    public:
        virtual ~MapRendererKeyedSymbolsResource();

        friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_KEYED_SYMBOLS_RESOURCE_H_)
