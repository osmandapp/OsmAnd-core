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
    _p->setOwnerIsLost();
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
    if (_polygonP)
        _polygonP->unregisterSymbolsGroup(this);
}

const OsmAnd::Polygon* OsmAnd::Polygon::SymbolsGroup::getPolygon() const
{
    if (_polygonP)
        return _polygonP->owner;
    return nullptr;
}

bool OsmAnd::Polygon::SymbolsGroup::updatesPresent()
{
    if (_polygonP)
        return _polygonP->hasUnappliedChanges();

    return false;
}

bool OsmAnd::Polygon::SymbolsGroup::supportsResourcesRenew()
{
    return true;
}

OsmAnd::IUpdatableMapSymbolsGroup::UpdateResult OsmAnd::Polygon::SymbolsGroup::update(const MapState& mapState)
{
    UpdateResult result = UpdateResult::None;
    if (_polygonP)
    {
        _polygonP->update(mapState);
        
        bool hasPropertiesChanges = _polygonP->hasUnappliedChanges();
        bool hasPrimitiveChanges = _polygonP->hasUnappliedPrimitiveChanges();
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
        
        _polygonP->applyChanges();
    }

    return result;
}
