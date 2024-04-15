#include "PolygonBuilder_P.h"
#include "PolygonBuilder.h"

#include "QtCommon.h"

#include "Polygon.h"
#include "Polygon_P.h"
#include "PolygonsCollection.h"
#include "PolygonsCollection_P.h"
#include "Utilities.h"

OsmAnd::PolygonBuilder_P::PolygonBuilder_P(PolygonBuilder* const owner_)
    : _isHidden(false)
    , _polygonId(0)
    , _baseOrder(std::numeric_limits<int>::min())
    , _circleRadiusInMeters(NAN)
    , _direction(0.0f)
    , owner(owner_)
{
}

OsmAnd::PolygonBuilder_P::~PolygonBuilder_P()
{
}

bool OsmAnd::PolygonBuilder_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);

    return _isHidden;
}

void OsmAnd::PolygonBuilder_P::setIsHidden(const bool hidden)
{
    QWriteLocker scopedLocker(&_lock);

    _isHidden = hidden;
}

int OsmAnd::PolygonBuilder_P::getPolygonId() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _polygonId;
}

void OsmAnd::PolygonBuilder_P::setPolygonId(const int polygonId)
{
    QWriteLocker scopedLocker(&_lock);
    
    _polygonId = polygonId;
}

int OsmAnd::PolygonBuilder_P::getBaseOrder() const
{
    QReadLocker scopedLocker(&_lock);

    return _baseOrder;
}

void OsmAnd::PolygonBuilder_P::setBaseOrder(const int baseOrder)
{
    QWriteLocker scopedLocker(&_lock);

    _baseOrder = baseOrder;
}

OsmAnd::FColorARGB OsmAnd::PolygonBuilder_P::getFillColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _fillColor;
}

void OsmAnd::PolygonBuilder_P::setFillColor(const FColorARGB fillColor)
{
    QWriteLocker scopedLocker(&_lock);

    _fillColor = fillColor;
}

QVector<OsmAnd::PointI> OsmAnd::PolygonBuilder_P::getPoints() const
{
    QReadLocker scopedLocker(&_lock);

    return _points;
}

void OsmAnd::PolygonBuilder_P::setPoints(const QVector<OsmAnd::PointI> points)
{
    QWriteLocker scopedLocker(&_lock);

    _points = points;
}

void OsmAnd::PolygonBuilder_P::setCircle(const OsmAnd::PointI& center, const double radiusInMeters)
{
    QWriteLocker scopedLocker(&_lock);

    _circleCenter = center;
    _circleRadiusInMeters = radiusInMeters;
}

std::shared_ptr<OsmAnd::Polygon> OsmAnd::PolygonBuilder_P::buildAndAddToCollection(
    const std::shared_ptr<PolygonsCollection>& collection)
{
    QReadLocker scopedLocker(&_lock);

    // Construct map symbols group for this polygon
    const std::shared_ptr<Polygon> polygon(build());

    // Add polygon to collection and return it if adding was successful
    if (!collection->_p->addPolygon(polygon))
        return nullptr;
    
    return polygon;
}

std::shared_ptr<OsmAnd::Polygon> OsmAnd::PolygonBuilder_P::build()
{
    QReadLocker scopedLocker(&_lock);
    
    // Construct map symbols group for this polygon
    const std::shared_ptr<Polygon> polygon(new Polygon(
                                                          _polygonId,
                                                          _baseOrder,
                                                          _fillColor));
    polygon->setIsHidden(_isHidden);
    if (!qIsNaN(_circleRadiusInMeters))
        polygon->setCircle(_circleCenter, _circleRadiusInMeters);
    else
        polygon->setPoints(_points);
    polygon->applyChanges();
    
    return polygon;
}
