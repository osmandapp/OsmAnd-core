#include "BinaryMapStaticSymbolsProvider_P.h"
#include "BinaryMapStaticSymbolsProvider.h"

#include "BinaryMapDataProvider.h"
#include "RasterizerEnvironment.h"
#include "Rasterizer.h"
#include "RasterizedSymbolsGroup.h"
#include "RasterizedPinnedSymbol.h"
#include "RasterizedSymbolOnPath.h"
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
    const IQueryController* const queryController)
{
    return obtainData(tileId, zoom, outTiledData, nullptr, queryController);
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
    std::shared_ptr<const MapTiledData > dataTile_;
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
        const auto constructedGroup = new Group(rasterizedGroup->mapObject);
        std::shared_ptr<const MapSymbolsGroup> group(constructedGroup);

        // Convert all symbols inside group
        for (const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            if (const auto pinnedSymbol = std::dynamic_pointer_cast<const RasterizedPinnedSymbol>(rasterizedSymbol))
            {
                const auto symbol = new SpriteMapSymbol(
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
