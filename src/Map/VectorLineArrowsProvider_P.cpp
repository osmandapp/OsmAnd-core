#include "VectorLineArrowsProvider_P.h"
#include "VectorLineArrowsProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapMarker.h"

OsmAnd::VectorLineArrowsProvider_P::VectorLineArrowsProvider_P(
    VectorLineArrowsProvider* const owner_,
    const std::shared_ptr<VectorLinesCollection>& collection)
	: _linesCollection(collection)
	, _markersCollection(std::make_shared<MapMarkersCollection>())
	, owner(owner_)
{
}

OsmAnd::VectorLineArrowsProvider_P::~VectorLineArrowsProvider_P()
{
    for (const auto& line : _linesCollection->getLines())
        line->updatedObservable.detach(reinterpret_cast<IObservable::Tag>(this));
}

void OsmAnd::VectorLineArrowsProvider_P::init()
{
    const auto selfWeak = std::weak_ptr<VectorLineArrowsProvider_P>(shared_from_this());
    for (const auto& line : _linesCollection->getLines())
    {
        line->updatedObservable.attach(reinterpret_cast<IObservable::Tag>(this),
            [selfWeak]
            (const VectorLine* const vectorLine)
            {
                const auto self = selfWeak.lock();
                if (self)
                    self->rebuildArrows();
            });
    }
    rebuildArrows();
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::VectorLineArrowsProvider_P::getMarker(int markerId) const
{
    for (const auto& marker : _markersCollection->getMarkers())
        if (marker->markerId == markerId)
            return marker;

    return nullptr;
}

QList<std::shared_ptr<OsmAnd::MapMarker>> OsmAnd::VectorLineArrowsProvider_P::getLineMarkers(int lineId) const
{
    QList<std::shared_ptr<OsmAnd::MapMarker>> res;
    for (const auto& marker : _markersCollection->getMarkers())
        if ((marker->markerId & 0x7FFF) == lineId)
            res.push_back(marker);

    return res;
}

void OsmAnd::VectorLineArrowsProvider_P::rebuildArrows()
{
    QWriteLocker scopedLocker(&_markersLock);

    for (const auto& line : _linesCollection->getLines())
    {
        if (!line)
            continue;

        int lineId = line->lineId;
        if (line->isHidden())
        {
            for (const auto& marker : getLineMarkers(lineId))
                _markersCollection->removeMarker(marker);

            continue;
        }

        const auto& symbolsData = line->getArrowsOnPath();
        int baseOrder = line->baseOrder - 100;
        int i = 0;
        for (const auto& symbolInfo : symbolsData)
        {
            int markerId = lineId | (i++ << 15);
            const auto& marker = getMarker(markerId);
            if (marker)
            {
                marker->setIsHidden(false);
                marker->setPosition(symbolInfo.position31);
                if (line->pathIconOnSurface)
                {
                    marker->setOnMapSurfaceIconDirection(
                        reinterpret_cast<OsmAnd::MapMarker::OnSurfaceIconKey>(1),
                        OsmAnd::Utilities::normalizedAngleDegrees(symbolInfo.direction));
                }
            }
            else
            {
                OsmAnd::MapMarkerBuilder builder;
                const auto markerKey = reinterpret_cast<OsmAnd::MapMarker::OnSurfaceIconKey>(1);
                const auto pointImage = SingleSkImage(line->getPointImage());
                if (line->pathIconOnSurface)
                    builder.addOnMapSurfaceIcon(markerKey, pointImage);
                else
                    builder.setPinIcon(pointImage);
                
                builder.setMarkerId(markerId);
                builder.setBaseOrder(baseOrder);
                builder.setIsHidden(false);
                builder.setPosition(symbolInfo.position31);
                builder.setIsAccuracyCircleVisible(false);
                const auto newMarker = builder.buildAndAddToCollection(_markersCollection);
                if (line->pathIconOnSurface)
                    newMarker->setOnMapSurfaceIconDirection(markerKey, symbolInfo.direction);
            }
        }
        for (const auto& marker : getLineMarkers(lineId))
        {
            int instanceId = marker->markerId >> 15;
            if (instanceId >= i)
                marker->setIsHidden(true);
        }
    }
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
