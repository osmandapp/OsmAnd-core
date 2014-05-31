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

    private:
    protected:
        IMapTiledSymbolsProvider();
    public:
        virtual ~IMapTiledSymbolsProvider();
    };

    class OSMAND_CORE_API MapTiledSymbols : public MapTiledData
    {
    private:
    protected:
        MapTiledSymbols(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups, const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~MapTiledSymbols();

        const QList< std::shared_ptr<const MapSymbolsGroup> > symbolsGroups;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_SYMBOLS_PROVIDER_H_)
