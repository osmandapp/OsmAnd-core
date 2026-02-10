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

void OsmAnd::VectorLineArrowsProvider_P::removeLineMarkers(int lineId) const
{
    QWriteLocker scopedLocker(&_markersLock);

    _markersCollection->removeMarkersByGroupId(lineId);
}

void OsmAnd::VectorLineArrowsProvider_P::removeAllMarkers() const
{
    QWriteLocker scopedLocker(&_markersLock);

    _markersCollection->removeAllMarkers();
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
            _markersCollection->removeMarkersByGroupId(lineId);
            continue;
        }

        const auto& symbolsData = line->getArrowsOnPath();
        int baseOrder = line->baseOrder - 100;
        int i = 0;
        for (const auto& symbolInfo : symbolsData)
        {
            const auto start = line->getStartingDistance() + line->getArrowStartingGap();
            int markerId = i++;
            const auto& marker = _markersCollection->getMarkerById(markerId, lineId);
            if (marker)
            {
                marker->setIsHidden(symbolInfo.distance < start);
                marker->setPosition(symbolInfo.position31);
                marker->setHeight(symbolInfo.elevation);
                marker->setElevationScaleFactor(symbolInfo.elevationScaleFactor);
                marker->setAdjustElevationToVectorObject(true);
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
                builder.setGroupId(lineId);
                builder.setUpdateAfterCreated(true);
                builder.setBaseOrder(baseOrder);
                builder.setIsHidden(symbolInfo.distance < start);
                builder.setPosition(symbolInfo.position31);
                builder.setHeight(symbolInfo.elevation);
                builder.setElevationScaleFactor(symbolInfo.elevationScaleFactor);
                builder.setIsAccuracyCircleVisible(false);
                const auto newMarker = builder.buildAndAddToCollection(_markersCollection);
                newMarker->setAdjustElevationToVectorObject(true);
                if (line->pathIconOnSurface)
                    newMarker->setOnMapSurfaceIconDirection(markerKey, symbolInfo.direction);
            }
        }
        const auto total = _markersCollection->getMarkersCountByGroupId(lineId);
        for (int markerId = i; markerId < total; markerId++)
        {
            _markersCollection->removeMarkerById(markerId, lineId);
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
