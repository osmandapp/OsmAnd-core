#ifndef _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapTiledSymbolsProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY(IMapTiledSymbolsProvider);

    public:
        OSMAND_CALLABLE(FilterCallback,
            bool,
            const IMapTiledSymbolsProvider* const provider,
            const std::shared_ptr<const MapSymbolsGroup>& symbolsGroup);

    private:
    protected:
        IMapTiledSymbolsProvider();
    public:
        virtual ~IMapTiledSymbolsProvider();

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        virtual bool obtainData(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const FilterCallback filterCallback = nullptr,
            const IQueryController* const queryController = nullptr) = 0;
    };

    class OSMAND_CORE_API TiledMapSymbolsData : public MapTiledData
    {
    private:
    protected:
    public:
        TiledMapSymbolsData(const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups, const TileId tileId, const ZoomLevel zoom);
        virtual ~TiledMapSymbolsData();

        QList< std::shared_ptr<MapSymbolsGroup> > symbolsGroups;
        virtual void releaseConsumableContent();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_)
