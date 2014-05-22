#include "OfflineMapStaticSymbolProvider_P.h"
#include "OfflineMapStaticSymbolProvider.h"

#include "OfflineMapDataProvider.h"
#include "OfflineMapDataTile.h"
#include "RasterizerEnvironment.h"
#include "Rasterizer.h"
#include "RasterizedSymbolsGroup.h"
#include "RasterizedPinnedSymbol.h"
#include "RasterizedSymbolOnPath.h"
#include "MapObject.h"
#include "Utilities.h"

OsmAnd::OfflineMapStaticSymbolProvider_P::OfflineMapStaticSymbolProvider_P(OfflineMapStaticSymbolProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::OfflineMapStaticSymbolProvider_P::~OfflineMapStaticSymbolProvider_P()
{
}

bool OsmAnd::OfflineMapStaticSymbolProvider_P::obtainSymbols(
    const TileId tileId, const ZoomLevel zoom,
    std::shared_ptr<const MapSymbolsTile>& outTile,
    const IMapSymbolProvider::FilterCallback filterCallback)
{
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

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
    rasterizer.rasterizeSymbolsWithoutPaths(rasterizedSymbolsGroups,
        [this, tileBBox31, filterCallback]
        (const std::shared_ptr<const Model::MapObject>& mapObject) -> bool
        {
            if (filterCallback == nullptr)
                return true;

            const auto isShareable =
                (mapObject->section != owner->dataProvider->rasterizerEnvironment->dummyMapSection) &&
                !tileBBox31.contains(mapObject->bbox31);

            return filterCallback(owner, mapObject, isShareable);
        }, nullptr);

    // Convert results
    QList< std::shared_ptr<const MapSymbolsGroup> > symbolsGroups;
    for (const auto& rasterizedGroup : constOf(rasterizedSymbolsGroups))
    {
        const auto& mapObject = rasterizedGroup->mapObject;
        const auto isShareable =
            (mapObject->section != owner->dataProvider->rasterizerEnvironment->dummyMapSection) &&
            !tileBBox31.contains(mapObject->bbox31);

        // Create group
        const auto constructedGroup = new MapSymbolsGroup(rasterizedGroup->mapObject);
        std::shared_ptr<const MapSymbolsGroup> group(constructedGroup);

        // Convert all symbols inside group
        for (const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            if (const auto pinnedSymbol = std::dynamic_pointer_cast<const RasterizedPinnedSymbol>(rasterizedSymbol))
            {
                const auto symbol = new PinnedMapSymbol(
                    group,
                    isShareable,
                    pinnedSymbol->bitmap,
                    pinnedSymbol->order,
                    pinnedSymbol->content,
                    pinnedSymbol->languageId,
                    pinnedSymbol->minDistance,
                    pinnedSymbol->location31,
                    pinnedSymbol->offset);
                assert(static_cast<bool>(symbol->bitmap));
                constructedGroup->symbols.push_back(qMove(std::shared_ptr<const MapSymbol>(symbol)));
            }
            else if (const auto symbolOnPath = std::dynamic_pointer_cast<const RasterizedSymbolOnPath>(rasterizedSymbol))
            {
                const auto symbol = new OnPathMapSymbol(
                    group,
                    isShareable,
                    symbolOnPath->bitmap,
                    symbolOnPath->order,
                    symbolOnPath->content,
                    symbolOnPath->languageId,
                    symbolOnPath->minDistance,
                    mapObject->points31,
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

OsmAnd::OfflineMapStaticSymbolProvider_P::Tile::Tile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_, const std::shared_ptr<const OfflineMapDataTile>& dataTile_)
    : MapSymbolsTile(symbolsGroups_)
    , dataTile(dataTile_)
{
}

OsmAnd::OfflineMapStaticSymbolProvider_P::Tile::~Tile()
{
}

void OsmAnd::OfflineMapStaticSymbolProvider_P::Tile::releaseNonRetainedData()
{
    _symbolsGroups.clear();
}
