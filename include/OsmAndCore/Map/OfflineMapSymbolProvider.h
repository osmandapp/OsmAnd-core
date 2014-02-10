#ifndef _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_H_
#define _OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapSymbolProvider.h>

namespace OsmAnd
{
    class OfflineMapDataProvider;

    class OfflineMapSymbolProvider_P;
    class OSMAND_CORE_API OfflineMapSymbolProvider : public IMapSymbolProvider
    {
        Q_DISABLE_COPY(OfflineMapSymbolProvider);
    private:
        const std::unique_ptr<OfflineMapSymbolProvider_P> _d;
    protected:
    public:
        OfflineMapSymbolProvider(const std::shared_ptr<OfflineMapDataProvider>& dataProvider);
        virtual ~OfflineMapSymbolProvider();

        const std::shared_ptr<OfflineMapDataProvider> dataProvider;

        virtual bool obtainSymbols(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapSymbolsTile>& outTile,
            std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter = nullptr);

        virtual bool canSymbolsBeSharedFrom(const std::shared_ptr<const Model::MapObject>& mapObject);
    };

}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_SYMBOL_PROVIDER_H_)
