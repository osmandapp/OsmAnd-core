#include "MarkersSymbolProvider.h"
#include "MarkersSymbolProvider_P.h"

OsmAnd::MarkersSymbolProvider::MarkersSymbolProvider()
    : _p(new MarkersSymbolProvider_P(this))
{
}

OsmAnd::MarkersSymbolProvider::~MarkersSymbolProvider()
{
}

std::shared_ptr<OsmAnd::MarkersSymbolProvider::Marker> OsmAnd::MarkersSymbolProvider::createMarker(
    const QString& name,
    const PointI location31,
    const float azimuth /*= 0.0f*/,
    const double areaRadius /*= 0.0*/,
    const SkColor areaBaseColor /*= SK_ColorTRANSPARENT*/,
    const std::shared_ptr<const SkBitmap>& pinIcon /*= nullptr*/,
    const std::shared_ptr<const SkBitmap>& surfaceIcon /*= nullptr*/)
{
    return _p->createMarker(name, location31, azimuth, areaRadius, areaBaseColor, pinIcon, surfaceIcon);
}

QList< std::shared_ptr<OsmAnd::MarkersSymbolProvider::Marker> > OsmAnd::MarkersSymbolProvider::getMarkersWithName(const QString& name) const
{
    return _p->getMarkersWithName(name);
}

QMultiHash< QString, std::shared_ptr<OsmAnd::MarkersSymbolProvider::Marker> > OsmAnd::MarkersSymbolProvider::getMarkers() const
{
    return _p->getMarkers();
}

bool OsmAnd::MarkersSymbolProvider::removeMarker(const std::shared_ptr<Marker>& marker)
{
    return _p->removeMarker(marker);
}

int OsmAnd::MarkersSymbolProvider::removeMarkersWithName(const QString& name)
{
    return _p->removeMarkersWithName(name);
}

int OsmAnd::MarkersSymbolProvider::removeAllMarkers()
{
    return _p->removeAllMarkers();
}

bool OsmAnd::MarkersSymbolProvider::obtainSymbols(QList< std::shared_ptr<const MapSymbolsGroup> >& outSymbolGroups, const FilterCallback filterCallback /*= nullptr*/)
{
    return _p->obtainSymbols(outSymbolGroups, filterCallback);
}

QSet<OsmAnd::MarkersSymbolProvider::Key> OsmAnd::MarkersSymbolProvider::getKeys() const
{
    return QSet<OsmAnd::MarkersSymbolProvider::Key>();
}
