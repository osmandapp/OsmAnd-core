#include "VectorLineBuilder_P.h"
#include "VectorLineBuilder.h"

#include "QtCommon.h"

#include "VectorLine.h"
#include "VectorLine_P.h"
#include "VectorLinesCollection.h"
#include "VectorLinesCollection_P.h"
#include "Utilities.h"

OsmAnd::VectorLineBuilder_P::VectorLineBuilder_P(VectorLineBuilder* const owner_)
    : _isHidden(false)
    , _lineId(0)
    , _baseOrder(std::numeric_limits<int>::min())
    , _lineWidth(3.0)
    , _direction(0.0f)
    , owner(owner_)
{
}

OsmAnd::VectorLineBuilder_P::~VectorLineBuilder_P()
{
}

bool OsmAnd::VectorLineBuilder_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);

    return _isHidden;
}

void OsmAnd::VectorLineBuilder_P::setIsHidden(const bool hidden)
{
    QWriteLocker scopedLocker(&_lock);

    _isHidden = hidden;
}

int OsmAnd::VectorLineBuilder_P::getLineId() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lineId;
}

void OsmAnd::VectorLineBuilder_P::setLineId(const int lineId)
{
    QWriteLocker scopedLocker(&_lock);
    
    _lineId = lineId;
}

int OsmAnd::VectorLineBuilder_P::getBaseOrder() const
{
    QReadLocker scopedLocker(&_lock);

    return _baseOrder;
}

void OsmAnd::VectorLineBuilder_P::setBaseOrder(const int baseOrder)
{
    QWriteLocker scopedLocker(&_lock);

    _baseOrder = baseOrder;
}

double OsmAnd::VectorLineBuilder_P::getLineWidth() const
{
    QReadLocker scopedLocker(&_lock);

    return _lineWidth;
}

void OsmAnd::VectorLineBuilder_P::setLineWidth(const double width)
{
    QWriteLocker scopedLocker(&_lock);

    _lineWidth = width;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder_P::getFillColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _fillColor;
}

void OsmAnd::VectorLineBuilder_P::setFillColor(const FColorARGB fillColor)
{
    QWriteLocker scopedLocker(&_lock);

    _fillColor = fillColor;
}

QVector<OsmAnd::PointI> OsmAnd::VectorLineBuilder_P::getPoints() const
{
    QReadLocker scopedLocker(&_lock);

    return _points;
}

void OsmAnd::VectorLineBuilder_P::setPoints(const QVector<OsmAnd::PointI> points)
{
    QWriteLocker scopedLocker(&_lock);

    _points = points;
}

std::shared_ptr<const SkBitmap> OsmAnd::VectorLineBuilder_P::getPathIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _pathIcon;
}

void OsmAnd::VectorLineBuilder_P::setPathIcon(const std::shared_ptr<const SkBitmap>& bitmap)
{
    QWriteLocker scopedLocker(&_lock);

    _pathIcon = bitmap;
}

float OsmAnd::VectorLineBuilder_P::getPathIconStep() const
{
    QReadLocker scopedLocker(&_lock);

    return _pathIconStep;
}

void OsmAnd::VectorLineBuilder_P::setPathIconStep(const float step)
{
    QWriteLocker scopedLocker(&_lock);

    _pathIconStep = step;
}

std::shared_ptr<OsmAnd::VectorLine> OsmAnd::VectorLineBuilder_P::buildAndAddToCollection(
    const std::shared_ptr<VectorLinesCollection>& collection)
{
    QReadLocker scopedLocker(&_lock);

    // Construct map symbols group for this line
    const std::shared_ptr<VectorLine> line(build());

    // Add line to collection and return it if adding was successful
    if (!collection->_p->addLine(line))
        return nullptr;
    
    return line;
}

std::shared_ptr<OsmAnd::VectorLine> OsmAnd::VectorLineBuilder_P::build()
{
    QReadLocker scopedLocker(&_lock);
    
    // Construct map symbols group for this line
    const std::shared_ptr<VectorLine> line(new VectorLine(
                                                          _lineId,
                                                          _baseOrder,
                                                          _lineWidth,
                                                          _pathIcon,
                                                          _pathIconStep));
    line->setFillColor(_fillColor);
    line->setIsHidden(_isHidden);
    line->setPoints(_points);
    line->applyChanges();
    
    return line;
}
