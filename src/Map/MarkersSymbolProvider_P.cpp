#include "MarkersSymbolProvider_P.h"
#include "MarkersSymbolProvider.h"

OsmAnd::MarkersSymbolProvider_P::MarkersSymbolProvider_P(MarkersSymbolProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MarkersSymbolProvider_P::~MarkersSymbolProvider_P()
{
}

std::shared_ptr<OsmAnd::MarkersSymbolProvider_P::Marker> OsmAnd::MarkersSymbolProvider_P::createMarker(
    const QString& name,
    const PointI location31,
    const float azimuth,
    const double areaRadius,
    const SkColor areaBaseColor,
    const std::shared_ptr<const SkBitmap>& pinIcon,
    const std::shared_ptr<const SkBitmap>& surfaceIcon)
{
    return nullptr;
}

QList< std::shared_ptr<OsmAnd::MarkersSymbolProvider_P::Marker> > OsmAnd::MarkersSymbolProvider_P::getMarkersWithName(const QString& name) const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markers.values(name);
}

QMultiHash< QString, std::shared_ptr<OsmAnd::MarkersSymbolProvider_P::Marker> > OsmAnd::MarkersSymbolProvider_P::getMarkers() const
{
    QReadLocker scopedLocker(&_markersLock);

    return detachedOf(_markers);
}

bool OsmAnd::MarkersSymbolProvider_P::removeMarker(const std::shared_ptr<Marker>& marker)
{
    return false;
}

int OsmAnd::MarkersSymbolProvider_P::removeMarkersWithName(const QString& name)
{
    return -1;
}

int OsmAnd::MarkersSymbolProvider_P::removeAllMarkers()
{
    return -1;
}

bool OsmAnd::MarkersSymbolProvider_P::obtainSymbols(QList< std::shared_ptr<const MapSymbolsGroup> >& outSymbolGroups, const FilterCallback filterCallback)
{
    return false;
}
