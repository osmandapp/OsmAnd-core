#include "OfflineMapSymbolProvider_P.h"
#include "OfflineMapSymbolProvider.h"

#include "OfflineMapDataProvider.h"
#include "OfflineMapDataTile.h"
#include "RasterizerEnvironment.h"
#include "Rasterizer.h"
#include "RasterizedSymbolsGroup.h"
#include "RasterizedPinnedSymbol.h"
#include "RasterizedSymbolOnPath.h"
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
    // Obtain offline map data tile
    std::shared_ptr< const OfflineMapDataTile > dataTile;
    owner->dataProvider->obtainTile(tileId, zoom, dataTile);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if (!dataTile || dataTile->nothingToRasterize)
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
    for(const auto& rasterizedGroup : constOf(rasterizedSymbolsGroups))
    {
        // Create group
        const auto constructedGroup = new MapSymbolsGroup(rasterizedGroup->mapObject);
        std::shared_ptr<const MapSymbolsGroup> group(constructedGroup);

        // Convert all symbols inside group
        for(const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            if (const auto pinnedSymbol = std::dynamic_pointer_cast<const RasterizedPinnedSymbol>(rasterizedSymbol))
            {
                const auto symbol = new MapPinnedSymbol(
                    group, constructedGroup->mapObject,
                    pinnedSymbol->bitmap,
                    pinnedSymbol->order,
                    pinnedSymbol->content,
                    pinnedSymbol->langId,
                    pinnedSymbol->minDistance,
                    pinnedSymbol->location31,
                    pinnedSymbol->offset);
                assert(static_cast<bool>(symbol->bitmap));
                constructedGroup->symbols.push_back(qMove(std::shared_ptr<const MapSymbol>(symbol)));
            }
            else if (const auto symbolOnPath = std::dynamic_pointer_cast<const RasterizedSymbolOnPath>(rasterizedSymbol))
            {
                const auto symbol = new MapSymbolOnPath(
                    group, constructedGroup->mapObject,
                    symbolOnPath->bitmap,
                    symbolOnPath->order,
                    symbolOnPath->content,
                    symbolOnPath->langId,
                    symbolOnPath->minDistance,
                    symbolOnPath->glyphsWidth);
                assert(static_cast<bool>(symbol->bitmap));
                constructedGroup->symbols.push_back(qMove(std::shared_ptr<const MapSymbol>(symbol)));
            }
        }

        // Add constructed group to output
        symbolsGroups.push_back(qMove(group));
    }

    // Create output tile
    outTile.reset(new Tile(symbolsGroups, dataTile));
    return true;
}

bool OsmAnd::OfflineMapSymbolProvider_P::canSymbolsBeSharedFrom(const std::shared_ptr<const Model::MapObject>& mapObject)
{
    return mapObject->section != owner->dataProvider->rasterizerEnvironment->dummyMapSection;
}

OsmAnd::OfflineMapSymbolProvider_P::Tile::Tile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_, const std::shared_ptr<const OfflineMapDataTile>& dataTile_)
    : MapSymbolsTile(symbolsGroups_)
    , dataTile(dataTile_)
{
}

OsmAnd::OfflineMapSymbolProvider_P::Tile::~Tile()
{
}

void OsmAnd::OfflineMapSymbolProvider_P::Tile::releaseNonRetainedData()
{
    _symbolsGroups.clear();
}
