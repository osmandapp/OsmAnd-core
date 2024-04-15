#include "Polygon.h"
#include "Polygon_P.h"

OsmAnd::Polygon::Polygon(
    const int polygonId_,
    const int baseOrder_,
    const FColorARGB fillColor_)
    : _p(new Polygon_P(this))
    , polygonId(polygonId_)
    , baseOrder(baseOrder_)
    , fillColor(fillColor_)
{
}

OsmAnd::Polygon::~Polygon()
{
}

bool OsmAnd::Polygon::isHidden() const
{
    return _p->isHidden();
}

void OsmAnd::Polygon::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);
}

QVector<OsmAnd::PointI> OsmAnd::Polygon::getPoints() const
{
    return _p->getPoints();
}

void OsmAnd::Polygon::setPoints(const QVector<OsmAnd::PointI>& points)
{
    _p->setPoints(points);
}

void OsmAnd::Polygon::setCircle(const OsmAnd::PointI& center, const double radiusInMeters)
{
    _p->setCircle(center, radiusInMeters);
}

void OsmAnd::Polygon::setZoomLevel(const OsmAnd::ZoomLevel zoomLevel, const bool hasElevationDataProvider /*= false*/)
{
    _p->setZoomLevel(zoomLevel, hasElevationDataProvider);
}

void OsmAnd::Polygon::generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol>& polygon) const
{
    _p->generatePrimitive(polygon);
}

bool OsmAnd::Polygon::hasUnappliedChanges() const
{
    return _p->hasUnappliedChanges();
}

bool OsmAnd::Polygon::applyChanges()
{
    return _p->applyChanges();
}

std::shared_ptr<OsmAnd::Polygon::SymbolsGroup> OsmAnd::Polygon::createSymbolsGroup(const MapState& mapState)
{
    return _p->createSymbolsGroup(mapState);
}

OsmAnd::Polygon::SymbolsGroup::SymbolsGroup(const std::shared_ptr<Polygon_P>& polygonP_)
    : _polygonP(polygonP_)
{
}

OsmAnd::Polygon::SymbolsGroup::~SymbolsGroup()
{
    if (const auto polygonP = _polygonP.lock())
        polygonP->unregisterSymbolsGroup(this);
}

const OsmAnd::Polygon* OsmAnd::Polygon::SymbolsGroup::getPolygon() const
{
    if (const auto polygonP = _polygonP.lock())
        return polygonP->owner;
    return nullptr;
}

bool OsmAnd::Polygon::SymbolsGroup::updatesPresent()
{
    if (const auto polygonP = _polygonP.lock())
        return polygonP->hasUnappliedChanges();

    return false;
}

bool OsmAnd::Polygon::SymbolsGroup::supportsResourcesRenew()
{
    return true;
}

OsmAnd::IUpdatableMapSymbolsGroup::UpdateResult OsmAnd::Polygon::SymbolsGroup::update(
    const MapState& mapState, IMapRenderer& mapRenderer)
{
    UpdateResult result = UpdateResult::None;
    if (const auto polygonP = _polygonP.lock())
    {
        polygonP->update(mapState);
        
        bool hasPropertiesChanges = polygonP->hasUnappliedChanges();
        bool hasPrimitiveChanges = polygonP->hasUnappliedPrimitiveChanges();
        if (hasPropertiesChanges && hasPrimitiveChanges)
        {
            result = UpdateResult::All;
        }
        else if (hasPropertiesChanges)
        {
            result = UpdateResult::Properties;
        }
        else if (hasPrimitiveChanges)
        {
            result = UpdateResult::Primitive;
        }
        
        polygonP->applyChanges();
    }

    return result;
}
