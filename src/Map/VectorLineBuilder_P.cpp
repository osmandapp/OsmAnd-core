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
    , _jointStyle(VectorLine::JointStyle::ROUND)
    , _outlineWidth(0)
    , _lineWidth(3.0)
    , _direction(0.0f)
    , _pathIconStep(-1.0)
    , _specialPathIconStep(-1.0)
    , _pathIconOnSurface(true)
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

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder_P::getOutlineColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _nearOutlineColor;
}

void OsmAnd::VectorLineBuilder_P::setOutlineColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    _nearOutlineColor = color;
    _farOutlineColor = color;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder_P::getNearOutlineColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _nearOutlineColor;
}

void OsmAnd::VectorLineBuilder_P::setNearOutlineColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    _nearOutlineColor = color;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder_P::getFarOutlineColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _farOutlineColor;
}

void OsmAnd::VectorLineBuilder_P::setFarOutlineColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    _farOutlineColor = color;
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

QVector<float> OsmAnd::VectorLineBuilder_P::getHeights() const
{
    QReadLocker scopedLocker(&_lock);

    return _heights;
}

void OsmAnd::VectorLineBuilder_P::setHeights(const QVector<float> heights)
{
    QWriteLocker scopedLocker(&_lock);

    _heights = heights;
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

QList<OsmAnd::FColorARGB> OsmAnd::VectorLineBuilder_P::getOutlineColorizationMapping() const
{
    QReadLocker scopedLocker(&_lock);

    return _outlineColorizationMapping;
}

void OsmAnd::VectorLineBuilder_P::setOutlineColorizationMapping(const QList<OsmAnd::FColorARGB> &colorizationMapping)
{
    QWriteLocker scopedLocker(&_lock);

    _outlineColorizationMapping = colorizationMapping;
}

sk_sp<const SkImage> OsmAnd::VectorLineBuilder_P::getPathIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _pathIcon;
}

void OsmAnd::VectorLineBuilder_P::setPathIcon(const SingleSkImage& image)
{
    QWriteLocker scopedLocker(&_lock);

    _pathIcon = image.sp;
}

sk_sp<const SkImage> OsmAnd::VectorLineBuilder_P::getSpecialPathIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _specialPathIcon;
}

void OsmAnd::VectorLineBuilder_P::setSpecialPathIcon(const SingleSkImage& image)
{
    QWriteLocker scopedLocker(&_lock);

    _specialPathIcon = image.sp;
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

float OsmAnd::VectorLineBuilder_P::getSpecialPathIconStep() const
{
    QReadLocker scopedLocker(&_lock);

    return _specialPathIconStep;
}

void OsmAnd::VectorLineBuilder_P::setSpecialPathIconStep(const float step)
{
    QWriteLocker scopedLocker(&_lock);

    _specialPathIconStep = step;
}

bool OsmAnd::VectorLineBuilder_P::isPathIconOnSurface() const
{
    QReadLocker scopedLocker(&_lock);

    return _pathIconOnSurface;
}

void OsmAnd::VectorLineBuilder_P::setPathIconOnSurface(const bool onSurface)
{
    QWriteLocker scopedLocker(&_lock);

    _pathIconOnSurface = onSurface;
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

void OsmAnd::VectorLineBuilder_P::setEndCapStyle(const VectorLine::EndCapStyle endCapStyle)
{
    QWriteLocker scopedLocker(&_lock);

    _endCapStyle = endCapStyle;
}

void OsmAnd::VectorLineBuilder_P::setJointStyle(const VectorLine::JointStyle jointStyle)
{
    QWriteLocker scopedLocker(&_lock);

    _jointStyle = jointStyle;
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
                                                          _pathIconOnSurface,
                                                          _screenScale,
                                                          _endCapStyle,
                                                          _jointStyle));
    line->setLineWidth(_lineWidth);
    line->setShowArrows(_showArrows);
    line->setFillColor(_fillColor);
    line->setIsHidden(_isHidden);
    line->setApproximationEnabled(_isApproximationEnabled);
    line->setPoints(_points);
    line->setHeights(_heights);
    line->setColorizationMapping(_colorizationMapping);
    line->setOutlineColorizationMapping(_outlineColorizationMapping);
    line->setLineDash(_dashPattern);
    line->setOutlineWidth(_outlineWidth);
    line->setNearOutlineColor(_nearOutlineColor);
    line->setFarOutlineColor(_farOutlineColor);
    line->setPathIconStep(_pathIconStep);
    line->setSpecialPathIconStep(_specialPathIconStep);
    line->setColorizationScheme(_colorizationScheme);
    line->applyChanges();
    
    return line;
}
