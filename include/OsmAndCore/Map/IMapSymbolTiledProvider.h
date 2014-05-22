#ifndef _OSMAND_CORE_I_MAP_SYMBOL_TILED_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_SYMBOL_TILED_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapSymbolProvider.h>

namespace OsmAnd
{
    class MapSymbolsTile;
    class OSMAND_CORE_API IMapSymbolTiledProvider : public IMapSymbolProvider
    {
        Q_DISABLE_COPY(IMapSymbolTiledProvider);
    private:
    protected:
        IMapSymbolTiledProvider();
    public:
        virtual ~IMapSymbolTiledProvider();

        virtual bool obtainSymbols(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapSymbolsTile>& outTile,
            const FilterCallback filterCallback = nullptr) = 0;
    };

    class OSMAND_CORE_API MapSymbolsTile
    {
    private:
    protected:
        MapSymbolsTile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups);

        QList< std::shared_ptr<const MapSymbolsGroup> > _symbolsGroups;
    public:
        virtual ~MapSymbolsTile();

        const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_SYMBOL_TILED_PROVIDER_H_)
