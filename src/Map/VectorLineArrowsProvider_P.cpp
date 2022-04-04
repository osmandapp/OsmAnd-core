#include "VectorLineArrowsProvider_P.h"
#include "VectorLineArrowsProvider.h"

#include "MapDataProviderHelpers.h"
#include "BillboardRasterMapSymbol.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "VectorLinesCollection.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "Logging.h"

OsmAnd::VectorLineArrowsProvider_P::VectorLineArrowsProvider_P(VectorLineArrowsProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::VectorLineArrowsProvider_P::~VectorLineArrowsProvider_P()
{
}

bool OsmAnd::VectorLineArrowsProvider_P::getVectorLineDataVersion(const std::shared_ptr<VectorLine>& vectorLine, int& version) const
{
    QReadLocker scopedLocker(&_lock);

    const auto citVersion = _lineVersions.constFind(vectorLine);
    if (citVersion != _lineVersions.cend())
    {
        version = *citVersion;
        return true;
    }
    return false;
}

void OsmAnd::VectorLineArrowsProvider_P::generateArrowsOnPath(
    const AreaI& bbox31,
    const ZoomLevel& zoom,
    const std::shared_ptr<SymbolsGroup>& symbolsGroup,
    const std::shared_ptr<VectorLine>& vectorLine)
{
    if (symbolsGroup && vectorLine)
    {
        const auto arrowImage = vectorLine->getPointImage();
        if (!arrowImage)
            return;
        
        int order = vectorLine->baseOrder - 100;

        const auto& arrows = vectorLine->getArrowsOnPath();
        for (const auto& arrow : constOf(arrows))
        {
            if (bbox31.contains(arrow.position31))
            {
                const auto arrowSymbol = std::make_shared<OnSurfaceRasterMapSymbol>(symbolsGroup);
                arrowSymbol->order = order;
                arrowSymbol->image = arrowImage;
                arrowSymbol->size = PointI(arrowImage->width(), arrowImage->height());
                arrowSymbol->content = QString().asprintf(
                    "markerGroup(%p:%p)->arrowImage:%p",
                    this,
                    symbolsGroup.get(),
                    arrowImage.get());
                arrowSymbol->languageId = LanguageId::Invariant;
                arrowSymbol->position31 = arrow.position31;
                arrowSymbol->direction = arrow.direction;
                arrowSymbol->isHidden = false;
                symbolsGroup->symbols.push_back(arrowSymbol);
            }
        }
    }
}

bool OsmAnd::VectorLineArrowsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<VectorLineArrowsProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    if (request.zoom > owner->getMaxZoom() || request.zoom < owner->getMinZoom())
    {
        outData.reset();
        return true;
    }

    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
    QList< std::shared_ptr<MapSymbolsGroup> > mapSymbolsGroups;
    
    const auto& lines = owner->vectorLinesCollection->getLines();
    for (const auto& line : constOf(lines))
    {
        _lineVersions[line] = line->getVersion();
        const auto& mapSymbolsGroup = std::make_shared<VectorLineArrowsProvider::SymbolsGroup>(shared_from_this(), line);
        generateArrowsOnPath(tileBBox31, request.zoom, mapSymbolsGroup, line);
        if (!mapSymbolsGroup->symbols.empty())
            mapSymbolsGroups.push_back(mapSymbolsGroup);
        }

    if (mapSymbolsGroups.empty())
    {
        outData.reset();
    }
    else
    {
        outData.reset(new VectorLineArrowsProvider::Data(
            request.tileId,
            request.zoom,
            mapSymbolsGroups));
    }
    return true;
}

OsmAnd::VectorLineArrowsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata()
{
}

OsmAnd::VectorLineArrowsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
