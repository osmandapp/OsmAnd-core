#include "OfflineMapSymbolProvider_P.h"
#include "OfflineMapSymbolProvider.h"

#include "OfflineMapDataProvider.h"
#include "OfflineMapDataTile.h"
#include "Rasterizer.h"
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

bool OsmAnd::OfflineMapSymbolProvider_P::obtainSymbols( const TileId tileId, const ZoomLevel zoom, QList< std::shared_ptr<const MapSymbol> >& outSymbols )
{
    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Obtain offline map data tile
    std::shared_ptr< const OfflineMapDataTile > dataTile;
    owner->dataProvider->obtainTile(tileId, zoom, dataTile);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if(dataTile->nothingToRasterize)
    {
        outSymbols.clear();
        return true;
    }

    // Create rasterizer
    Rasterizer rasterizer(dataTile->rasterizerContext);

    // Rasterize symbols
    QList< std::shared_ptr<const RasterizedSymbol> > rasterizedSymbols;
    rasterizer.rasterizeSymbolsWithoutPaths(rasterizedSymbols);
    
    // Convert results
    for(auto itRasterizedSymbol = rasterizedSymbols.cbegin(); itRasterizedSymbol != rasterizedSymbols.cend(); ++itRasterizedSymbol)
    {
        const auto& rasterizedSymbol = *itRasterizedSymbol;

        // Create new map symbol
        const auto mapSymbol = new MapSymbol(
            rasterizedSymbol->mapObject->id,
            rasterizedSymbol->location31,
            zoom,
            rasterizedSymbol->icon,
            rasterizedSymbol->texts);

        outSymbols.push_back(qMove(std::shared_ptr<const MapSymbol>(mapSymbol)));
    }

    return true;
}
