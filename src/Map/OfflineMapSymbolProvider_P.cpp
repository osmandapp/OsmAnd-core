#include "OfflineMapSymbolProvider_P.h"
#include "OfflineMapSymbolProvider.h"

#include "OfflineMapDataProvider.h"
#include "OfflineMapDataTile.h"
#include "Rasterizer.h"
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
    rasterizer.rasterizeSymbols();// returns set of < MapObject ref, SkBitmap ref / text, SkBitmap ref / icon >
    //TODO: start extracting text primitives one by one, checking with filterById if one should be gained from specific mapobject

    //TODO: if filter was passed, rasterize the icon or text or whatever

    return false;
}
