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
#include "IMapKeyedSymbolsProvider.h"

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
        MapRendererKeyedSymbolsResource(
            MapRendererResourcesManager* owner,
            const KeyedEntriesCollection<Key, MapRendererBaseKeyedResource>& collection,
            const Key key);

        std::shared_ptr<IMapKeyedSymbolsProvider::Data> _sourceData;
        std::shared_ptr<MapSymbolsGroup> _mapSymbolsGroup;
        QHash< std::shared_ptr<const MapSymbol>, std::shared_ptr<const GPUAPI::ResourceInGPU> > _resourcesInGPU;

        virtual bool updatesPresent();
        virtual bool checkForUpdatesAndApply(const MapState& mapState);
        
        virtual bool supportsObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            bool& dataAvailable,
            const std::shared_ptr<const IQueryController>& queryController) Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const ObtainDataAsyncCallback callback,
            const std::shared_ptr<const IQueryController>& queryController) Q_DECL_OVERRIDE;

        virtual bool uploadToGPU() Q_DECL_OVERRIDE;
        virtual void unloadFromGPU() Q_DECL_OVERRIDE;
        virtual void lostDataInGPU() Q_DECL_OVERRIDE;
        virtual void releaseData() Q_DECL_OVERRIDE;
    public:
        virtual ~MapRendererKeyedSymbolsResource();

        std::shared_ptr<const GPUAPI::ResourceInGPU> getGpuResourceFor(const std::shared_ptr<const MapSymbol>& mapSymbol) const;

        friend class OsmAnd::MapRendererResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_KEYED_SYMBOLS_RESOURCE_H_)
