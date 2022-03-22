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
    , _showArrows(false)
    , _isApproximationEnabled(true)
    , _lineId(0)
    , _baseOrder(std::numeric_limits<int>::min())
    , _colorizationScheme(0)
    , _endCapStyle(VectorLine::EndCapStyle::ROUND)
    , _outlineWidth(0)
    , _lineWidth(3.0)
    , _direction(0.0f)
    , _pathIconStep(-1.0)
    , _screenScale(2)
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

bool OsmAnd::VectorLineBuilder_P::shouldShowArrows() const
{
    QReadLocker scopedLocker(&_lock);

    return _showArrows;
}

void OsmAnd::VectorLineBuilder_P::setShouldShowArrows(const bool showArrows)
{
    QWriteLocker scopedLocker(&_lock);

    _showArrows = showArrows;
}

bool OsmAnd::VectorLineBuilder_P::isApproximationEnabled() const
{
    QReadLocker scopedLocker(&_lock);

    return _isApproximationEnabled;
}

void OsmAnd::VectorLineBuilder_P::setApproximationEnabled(const bool enabled)
{
    QWriteLocker scopedLocker(&_lock);

    _isApproximationEnabled = enabled;
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

int OsmAnd::VectorLineBuilder_P::getColorizationScheme() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _colorizationScheme;
}

void OsmAnd::VectorLineBuilder_P::setColorizationScheme(const int colorizationScheme)
{
    QWriteLocker scopedLocker(&_lock);
    
    _colorizationScheme = colorizationScheme;
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

double OsmAnd::VectorLineBuilder_P::getOutlineWidth() const
{
    QReadLocker scopedLocker(&_lock);

    return _outlineWidth;
}

void OsmAnd::VectorLineBuilder_P::setOutlineWidth(const double width)
{
    QWriteLocker scopedLocker(&_lock);

    _outlineWidth = width;
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

std::vector<double> OsmAnd::VectorLineBuilder_P::getLineDash() const
{
    QReadLocker scopedLocker(&_lock);

    return _dashPattern;
}

void OsmAnd::VectorLineBuilder_P::setLineDash(const std::vector<double> dashPattern)
{
    QWriteLocker scopedLocker(&_lock);

    _dashPattern = dashPattern;
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

QList<OsmAnd::FColorARGB> OsmAnd::VectorLineBuilder_P::getColorizationMapping() const
{
    QReadLocker scopedLocker(&_lock);

    return _colorizationMapping;
}

void OsmAnd::VectorLineBuilder_P::setColorizationMapping(const QList<OsmAnd::FColorARGB> &colorizationMapping)
{
    QWriteLocker scopedLocker(&_lock);

    _colorizationMapping = colorizationMapping;
}

sk_sp<const SkImage> OsmAnd::VectorLineBuilder_P::getPathIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _pathIcon;
}

void OsmAnd::VectorLineBuilder_P::setPathIcon(const sk_sp<const SkImage>& image)
{
    QWriteLocker scopedLocker(&_lock);

    _pathIcon = image;
}

sk_sp<const SkImage> OsmAnd::VectorLineBuilder_P::getSpecialPathIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _specialPathIcon;
}

void OsmAnd::VectorLineBuilder_P::setSpecialPathIcon(const sk_sp<const SkImage>& image)
{
    QWriteLocker scopedLocker(&_lock);

    _specialPathIcon = image;
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

float OsmAnd::VectorLineBuilder_P::getScreenScale() const
{
    QReadLocker scopedLocker(&_lock);

    return _screenScale;
}

void OsmAnd::VectorLineBuilder_P::setScreenScale(const float screenScale)
{
    QWriteLocker scopedLocker(&_lock);

    _screenScale = screenScale;
}

void OsmAnd::VectorLineBuilder_P::setIconScale(const float iconScale)
{
    QWriteLocker scopedLocker(&_lock);

    _iconScale = iconScale;
}

void OsmAnd::VectorLineBuilder_P::setEndCapStyle(const VectorLine::EndCapStyle endCapStyle)
{
    QWriteLocker scopedLocker(&_lock);

    _endCapStyle = endCapStyle;
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
                                                          _pathIcon,
                                                          _specialPathIcon,
                                                          _pathIconStep,
                                                          _screenScale,
                                                          _iconScale,
                                                          _endCapStyle));
    line->setLineWidth(_lineWidth);
    line->setShowArrows(_showArrows);
    line->setFillColor(_fillColor);
    line->setIsHidden(_isHidden);
    line->setApproximationEnabled(_isApproximationEnabled);
    line->setPoints(_points);
    line->setColorizationMapping(_colorizationMapping);
    line->setLineDash(_dashPattern);
    line->setOutlineWidth(_outlineWidth);
    line->setColorizationScheme(_colorizationScheme);
    line->applyChanges();
    
    return line;
}
