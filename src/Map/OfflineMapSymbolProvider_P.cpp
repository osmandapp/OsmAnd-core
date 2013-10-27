#include "OfflineMapSymbolProvider_P.h"
#include "OfflineMapSymbolProvider.h"

#include "OfflineMapDataProvider.h"
#include "Utilities.h"

OsmAnd::OfflineMapSymbolProvider_P::OfflineMapSymbolProvider_P( OfflineMapSymbolProvider* owner_ )
    : owner(owner_)
{
}

OsmAnd::OfflineMapSymbolProvider_P::~OfflineMapSymbolProvider_P()
{
}

bool OsmAnd::OfflineMapSymbolProvider_P::obtainSymbols( const QSet<TileId>& tileIds, const ZoomLevel zoom, QList< std::shared_ptr<const MapSymbol> >& outSymbols, std::function<bool (const uint64_t)> filterById /*= nullptr*/ )
{
    for(auto itTileId = tileIds.cbegin(); itTileId != tileIds.cend(); ++itTileId)
    {
        const auto tileId = *itTileId;

        // Get bounding box that covers this tile
        const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

        // Obtain offline map data tile
        std::shared_ptr< const OfflineMapDataTile > dataTile;
        owner->dataProvider->obtainTile(tileId, zoom, dataTile);

        //TODO: start extracting text primitives one by one, checking with filterById if one should be gained from specific mapobject

        //TODO: if filter was passed, rasterize the icon or text or whatever
    }

    return false;
}
