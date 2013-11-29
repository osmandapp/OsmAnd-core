#include "OfflineMapSymbolProvider_P.h"
#include "OfflineMapSymbolProvider.h"

#include "OfflineMapDataProvider.h"
#include "OfflineMapDataTile.h"
#include "RasterizerEnvironment.h"
#include "Rasterizer.h"
#include "RasterizedSymbolsGroup.h"
#include "RasterizedSymbol.h"
#include "MapObject.h"
#include "Utilities.h"

OsmAnd::OfflineMapSymbolProvider_P::OfflineMapSymbolProvider_P( OfflineMapSymbolProvider* owner_ )
    : owner(owner_)
{
}

OsmAnd::OfflineMapSymbolProvider_P::~OfflineMapSymbolProvider_P()
{
}

bool OsmAnd::OfflineMapSymbolProvider_P::obtainSymbols(
    const TileId tileId, const ZoomLevel zoom,
    std::shared_ptr<const MapSymbolsTile>& outTile,
    std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter)
{
    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Obtain offline map data tile
    std::shared_ptr< const OfflineMapDataTile > dataTile;
    owner->dataProvider->obtainTile(tileId, zoom, dataTile);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if(dataTile->nothingToRasterize)
    {
        // Mark tile as empty
        outTile.reset();
        return true;
    }

    // Create rasterizer
    Rasterizer rasterizer(dataTile->rasterizerContext);

    // Rasterize symbols
    QList< std::shared_ptr<const RasterizedSymbolsGroup> > rasterizedSymbolsGroups;
    rasterizer.rasterizeSymbolsWithoutPaths(rasterizedSymbolsGroups, filter, nullptr);
    
    // Convert results
    QList< std::shared_ptr<const MapSymbolsGroup> > symbolsGroups;
    for(auto itRasterizedGroup = rasterizedSymbolsGroups.cbegin(); itRasterizedGroup != rasterizedSymbolsGroups.cend(); ++itRasterizedGroup)
    {
        const auto& rasterizedGroup = *itRasterizedGroup;

        // Create group
        const auto constructedGroup = new MapSymbolsGroup(rasterizedGroup->mapObject);
        std::shared_ptr<const MapSymbolsGroup> group(constructedGroup);

        // Convert all symbols inside group
        for(auto itRasterizedSymbol = rasterizedGroup->symbols.cbegin(); itRasterizedSymbol != rasterizedGroup->symbols.cend(); ++itRasterizedSymbol)
        {
            const auto& rasterizedSymbol = *itRasterizedSymbol;

            const auto symbol = new MapSymbol(
                group, constructedGroup->mapObject,
                rasterizedSymbol->order,
                rasterizedSymbol->location31,
                rasterizedSymbol->bitmap);
            constructedGroup->symbols.push_back(qMove(std::shared_ptr<const MapSymbol>(symbol)));
        }

        // Add constructed group to output
        symbolsGroups.push_back(qMove(group));
    }

    // Create output tile
    outTile.reset(new Tile(symbolsGroups, dataTile));
    return true;
}

OsmAnd::OfflineMapSymbolProvider_P::Tile::Tile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_, const std::shared_ptr<const OfflineMapDataTile>& dataTile_)
    : MapSymbolsTile(symbolsGroups_)
    , dataTile(dataTile_)
{
}

OsmAnd::OfflineMapSymbolProvider_P::Tile::~Tile()
{
}
