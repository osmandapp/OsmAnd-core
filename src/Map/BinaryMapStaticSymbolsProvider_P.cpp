#include "BinaryMapStaticSymbolsProvider_P.h"
#include "BinaryMapStaticSymbolsProvider.h"

#include "BinaryMapDataProvider.h"
#include "RasterizerEnvironment.h"
#include "Rasterizer.h"
#include "RasterizedSymbolsGroup.h"
#include "RasterizedSpriteSymbol.h"
#include "RasterizedOnPathSymbol.h"
#include "SpriteMapSymbol.h"
#include "OnPathMapSymbol.h"
#include "MapObject.h"
#include "Utilities.h"

OsmAnd::BinaryMapStaticSymbolsProvider_P::BinaryMapStaticSymbolsProvider_P(BinaryMapStaticSymbolsProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider_P::~BinaryMapStaticSymbolsProvider_P()
{
}

bool OsmAnd::BinaryMapStaticSymbolsProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<const MapTiledData>& outTiledData,
    const FilterCallback filterCallback,
    const IQueryController* const queryController)
{
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Obtain offline map data tile
    std::shared_ptr<const MapTiledData> dataTile_;
    owner->dataProvider->obtainData(tileId, zoom, dataTile_);
    const auto dataTile = std::static_pointer_cast<const BinaryMapDataTile>(dataTile_);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if (!dataTile_ || dataTile->nothingToRasterize)
    {
        // Mark tile as empty
        outTiledData.reset();
        return true;
    }

    // Create rasterizer
    Rasterizer rasterizer(dataTile->rasterizerContext);

    // Rasterize symbols and create symbols groups
    QList< std::shared_ptr<const RasterizedSymbolsGroup> > rasterizedSymbolsGroups;
    QHash< uint64_t, std::shared_ptr<MapSymbolsGroup> > preallocatedSymbolsGroups;
    const auto rasterizationFilter =
        [this, tileBBox31, filterCallback, &preallocatedSymbolsGroups]
        (const std::shared_ptr<const Model::MapObject>& mapObject) -> bool
        {
            const auto isShareable =
                (mapObject->section != owner->dataProvider->rasterizerEnvironment->dummyMapSection) &&
                !tileBBox31.contains(mapObject->bbox31);
            const std::shared_ptr<MapSymbolsGroup> preallocatedGroup(isShareable ? new MapSymbolsGroupShareableById(mapObject->id) : new MapSymbolsGroup());

            if (!filterCallback || filterCallback(owner, preallocatedGroup))
            {
                preallocatedSymbolsGroups.insert(mapObject->id, preallocatedGroup);
                return true;
            }
            return false;
        };
    rasterizer.rasterizeSymbolsWithoutPaths(rasterizedSymbolsGroups, rasterizationFilter, nullptr);

    // Convert results
    QList< std::shared_ptr<const MapSymbolsGroup> > symbolsGroups;
    for (const auto& rasterizedGroup : constOf(rasterizedSymbolsGroups))
    {
        const auto& mapObject = rasterizedGroup->mapObject;
        
        // Get preallocated group
        const auto citPreallocatedGroup = preallocatedSymbolsGroups.constFind(mapObject->id);
        assert(citPreallocatedGroup != preallocatedSymbolsGroups.cend());
        const auto group = *citPreallocatedGroup;
        const auto isShareable = (std::dynamic_pointer_cast<MapSymbolsGroupShareableById>(group) != nullptr);

        // Convert all symbols inside group
        for (const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            if (const auto pinnedSymbol = std::dynamic_pointer_cast<const RasterizedSpriteSymbol>(rasterizedSymbol))
            {
                const auto symbol = new SpriteMapSymbol(
                    group,
                    isShareable,
                    pinnedSymbol->bitmap,
                    pinnedSymbol->order,
                    MapSymbol::RegularIntersectionProcessing,
                    pinnedSymbol->content,
                    pinnedSymbol->languageId,
                    pinnedSymbol->minDistance,
                    pinnedSymbol->location31,
                    pinnedSymbol->offset);
                assert(static_cast<bool>(symbol->bitmap));
                group->symbols.push_back(qMove(std::shared_ptr<const MapSymbol>(symbol)));
            }
            else if (const auto symbolOnPath = std::dynamic_pointer_cast<const RasterizedOnPathSymbol>(rasterizedSymbol))
            {
                const auto symbol = new OnPathMapSymbol(
                    group,
                    isShareable,
                    symbolOnPath->bitmap,
                    symbolOnPath->order,
                    MapSymbol::RegularIntersectionProcessing,
                    symbolOnPath->content,
                    symbolOnPath->languageId,
                    symbolOnPath->minDistance,
                    mapObject->points31,
                    symbolOnPath->glyphsWidth);
                assert(static_cast<bool>(symbol->bitmap));
                group->symbols.push_back(qMove(std::shared_ptr<const MapSymbol>(symbol)));
            }
        }

        // Add constructed group to output
        symbolsGroups.push_back(qMove(group));
    }

    // Create output tile
    outTiledData.reset(new BinaryMapStaticSymbolsTile(dataTile, symbolsGroups, tileId, zoom));

    return true;
}

OsmAnd::BinaryMapStaticSymbolsProvider_P::Group::Group(const std::shared_ptr<const Model::MapObject>& mapObject_)
    : MapSymbolsGroupShareableById(mapObject_->id)
    , mapObject(mapObject_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider_P::Group::~Group()
{
}

QString OsmAnd::BinaryMapStaticSymbolsProvider_P::Group::getDebugTitle() const
{
    return QString(QLatin1String("MO %1,%2")).arg(id).arg(static_cast<int64_t>(id) / 2);
}
