#include "VectorLineArrowsProvider_P.h"
#include "VectorLineArrowsProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapMarker.h"

OsmAnd::VectorLineArrowsProvider_P::VectorLineArrowsProvider_P(
    VectorLineArrowsProvider* const owner_,
    const std::shared_ptr<VectorLinesCollection>& collection)
    : owner(owner_)
    , _linesCollection(collection)
    , _markersCollection(std::make_shared<MapMarkersCollection>())
{
    for (const auto& line : collection->getLines())
    {
        line->lineUpdatedObservable.attach(this,
            [this]
            (const VectorLine* const vectorLine)
            {
                rebuildArrows();
            });
    }
    rebuildArrows();
}

OsmAnd::VectorLineArrowsProvider_P::~VectorLineArrowsProvider_P()
{
}

void OsmAnd::VectorLineArrowsProvider_P::rebuildArrows()
{
    QWriteLocker scopedLocker(&_markersLock);
    
    const auto& markers = _markersCollection->getMarkers();
    int lineSymbolIdx = 0;
    int initialSymbolsCount = markers.size();
    for (const auto& line : _linesCollection->getLines())
    {
        const auto& symbolsData = line->getArrowsOnPath();
        int baseOrder = line->baseOrder - 100;
        for (const auto& symbolInfo : symbolsData)
        {
            if (lineSymbolIdx < initialSymbolsCount)
            {
                auto marker = markers[lineSymbolIdx++];
                marker->setPosition(symbolInfo.position31);
                marker->setOnMapSurfaceIconDirection(
                    reinterpret_cast<OsmAnd::MapMarker::OnSurfaceIconKey>(marker->markerId),
                    OsmAnd::Utilities::normalizedAngleDegrees(symbolInfo.direction));
                marker->setIsHidden(line->isHidden());
            }
            else
            {
                OsmAnd::MapMarkerBuilder builder;
                const auto markerKey = reinterpret_cast<OsmAnd::MapMarker::OnSurfaceIconKey>(markers.size());
                builder.addOnMapSurfaceIcon(markerKey, line->getPointImage());
                builder.setMarkerId(markers.size());
                builder.setBaseOrder(--baseOrder);
                builder.setIsHidden(line->isHidden());
                const auto& marker = builder.buildAndAddToCollection(_markersCollection);
                marker->setPosition(symbolInfo.position31);
                marker->setOnMapSurfaceIconDirection(markerKey, symbolInfo.direction);
                marker->setIsAccuracyCircleVisible(false);
            }
        }
    }
    while (lineSymbolIdx < initialSymbolsCount && markers.size() > lineSymbolIdx)
        _markersCollection->removeMarker(markers[lineSymbolIdx++]);
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::VectorLineArrowsProvider_P::getProvidedDataKeys() const
{
    QReadLocker scopedLocker(&_markersLock);
    
    return _markersCollection->getProvidedDataKeys();
}

bool OsmAnd::VectorLineArrowsProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData)
{
    QReadLocker scopedLocker(&_markersLock);

    return _markersCollection->obtainData(request, outData);
}

OsmAnd::ZoomLevel OsmAnd::VectorLineArrowsProvider_P::getMinZoom() const
{
    return _linesCollection->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::VectorLineArrowsProvider_P::getMaxZoom() const
{
    return _linesCollection->getMaxZoom();
}
