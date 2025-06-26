#include "VectorLine.h"
#include "VectorLine_P.h"

OsmAnd::VectorLine::VectorLine(
    const int lineId_,
    const int baseOrder_,
    const sk_sp<const SkImage>& pathIcon_/* = nullptr*/,
    const sk_sp<const SkImage>& specialPathIcon_/* = nullptr*/,
    const bool pathIconOnSurface_/* = true*/,
    const float screenScale_/* = 2*/,
    const VectorLine::EndCapStyle endCapStyle_/* = EndCapStyle::ROUND*/,
    const VectorLine::JointStyle jointStyle_/* = JointStyle::ROUND*/)
    : _p(new VectorLine_P(this))
    , lineId(lineId_)
    , baseOrder(baseOrder_)
    , pathIcon(pathIcon_)
    , specialPathIcon(specialPathIcon_)
    , pathIconOnSurface(pathIconOnSurface_)
    , screenScale(screenScale_)
    , endCapStyle(endCapStyle_)
    , jointStyle(jointStyle_)
{
}

OsmAnd::VectorLine::~VectorLine()
{
}

bool OsmAnd::VectorLine::isHidden() const
{
    return _p->isHidden();
}

void OsmAnd::VectorLine::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);
}

float OsmAnd::VectorLine::getStartingDistance() const
{
    return _p->getStartingDistance();
}

void OsmAnd::VectorLine::setStartingDistance(const float distanceInMeters)
{
    _p->setStartingDistance(distanceInMeters);
}

float OsmAnd::VectorLine::getArrowStartingGap() const
{
    return _p->getArrowStartingGap();
}

bool OsmAnd::VectorLine::showArrows() const
{
    return _p->showArrows();
}

void OsmAnd::VectorLine::setShowArrows(const bool showArrows)
{
    _p->setShowArrows(showArrows);
}

bool OsmAnd::VectorLine::isApproximationEnabled() const
{
    return _p->isApproximationEnabled();
}

void OsmAnd::VectorLine::setApproximationEnabled(const bool enabled)
{
    _p->setApproximationEnabled(enabled);
}

QVector<OsmAnd::PointI> OsmAnd::VectorLine::getPoints() const
{
    return _p->getPoints();
}

void OsmAnd::VectorLine::attachMarker(const std::shared_ptr<MapMarker>& marker)
{
    return _p->attachMarker(marker);
}

void OsmAnd::VectorLine::detachMarker(const std::shared_ptr<MapMarker>& marker)
{
    _p->detachMarker(marker);
}

void OsmAnd::VectorLine::setPoints(const QVector<OsmAnd::PointI>& points)
{
    _p->setPoints(points);
}

QList<float> OsmAnd::VectorLine::getHeights() const
{
    return _p->getHeights();
}

void OsmAnd::VectorLine::setHeights(const QList<float>& heights)
{
    _p->setHeights(heights);
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLine::getColorizationMapping() const
{
    return _p->getColorizationMapping();
}

void OsmAnd::VectorLine::setColorizationMapping(const QList<FColorARGB> &colorizationMapping)
{
    _p->setColorizationMapping(colorizationMapping);
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLine::getOutlineColorizationMapping() const
{
    return _p->getOutlineColorizationMapping();
}

void OsmAnd::VectorLine::setOutlineColorizationMapping(const QList<FColorARGB> &colorizationMapping)
{
    _p->setOutlineColorizationMapping(colorizationMapping);
}

double OsmAnd::VectorLine::getLineWidth() const
{
    return _p->getLineWidth();
}

void OsmAnd::VectorLine::setLineWidth(const double width)
{
    _p->setLineWidth(width);
}

double OsmAnd::VectorLine::getOutlineWidth() const
{
    return _p->getOutlineWidth();
}

void OsmAnd::VectorLine::setOutlineWidth(const double width)
{
    _p->setOutlineWidth(width);
}

OsmAnd::FColorARGB OsmAnd::VectorLine::getOutlineColor() const
{
    return _p->getOutlineColor();
}

void OsmAnd::VectorLine::setOutlineColor(const FColorARGB color)
{
    _p->setOutlineColor(color);
}

OsmAnd::FColorARGB OsmAnd::VectorLine::getNearOutlineColor() const
{
    return _p->getNearOutlineColor();
}

void OsmAnd::VectorLine::setNearOutlineColor(const FColorARGB color)
{
    _p->setNearOutlineColor(color);
}

OsmAnd::FColorARGB OsmAnd::VectorLine::getFarOutlineColor() const
{
    return _p->getFarOutlineColor();
}

void OsmAnd::VectorLine::setFarOutlineColor(const FColorARGB color)
{
    _p->setFarOutlineColor(color);
}

void OsmAnd::VectorLine::setColorizationScheme(const int colorizationScheme)
{
    _p->setColorizationScheme(colorizationScheme);
}

OsmAnd::FColorARGB OsmAnd::VectorLine::getFillColor() const
{
    return _p->getFillColor();
}

void OsmAnd::VectorLine::setFillColor(const FColorARGB color)
{
    _p->setFillColor(color);
}

bool OsmAnd::VectorLine::getElevatedLineVisibility() const
{
    return _p->getElevatedLineVisibility();
}

void OsmAnd::VectorLine::setElevatedLineVisibility(const bool visible)
{
    _p->setElevatedLineVisibility(visible);
}

bool OsmAnd::VectorLine::getSurfaceLineVisibility() const
{
    return _p->getSurfaceLineVisibility();
}

void OsmAnd::VectorLine::setSurfaceLineVisibility(const bool visible)
{
    _p->setSurfaceLineVisibility(visible);
}

float OsmAnd::VectorLine::getElevationScaleFactor() const
{
    return _p->getElevationScaleFactor();
}

void OsmAnd::VectorLine::setElevationScaleFactor(const float scaleFactor)
{
    _p->setElevationScaleFactor(scaleFactor);
}

float OsmAnd::VectorLine::getPathIconStep() const
{
    return _p->getPathIconStep();
}

void OsmAnd::VectorLine::setPathIconStep(const float step)
{
    _p->setPathIconStep(step);
}

float OsmAnd::VectorLine::getSpecialPathIconStep() const
{
    return _p->getSpecialPathIconStep();
}

void OsmAnd::VectorLine::setSpecialPathIconStep(const float step)
{
    _p->setSpecialPathIconStep(step);
}

std::vector<double> OsmAnd::VectorLine::getLineDash() const
{
    return _p->getLineDash();
}

void OsmAnd::VectorLine::setLineDash(const std::vector<double> dashPattern)
{
    _p->setLineDash(dashPattern);
}

sk_sp<const SkImage> OsmAnd::VectorLine::getPointImage() const
{
    return _p->getPointImage();
}

bool OsmAnd::VectorLine::hasUnappliedChanges() const
{
    return _p->hasUnappliedChanges();
}

bool OsmAnd::VectorLine::applyChanges()
{
    return _p->applyChanges();
}

std::shared_ptr<OsmAnd::VectorLine::SymbolsGroup> OsmAnd::VectorLine::createSymbolsGroup(const MapState& mapState)
{
    return _p->createSymbolsGroup(mapState);
}

const QList<OsmAnd::VectorLine::OnPathSymbolData> OsmAnd::VectorLine::getArrowsOnPath() const
{
    return _p->getArrowsOnPath();
}

OsmAnd::VectorLine::SymbolsGroup::SymbolsGroup(const std::shared_ptr<VectorLine_P>& vectorLineP_)
    : _vectorLineP(vectorLineP_)
{
}

OsmAnd::VectorLine::SymbolsGroup::~SymbolsGroup()
{
    if (const auto vectorLineP = _vectorLineP.lock())
        vectorLineP->unregisterSymbolsGroup(this);
}

const OsmAnd::VectorLine* OsmAnd::VectorLine::SymbolsGroup::getVectorLine() const
{
    if (const auto vectorLineP = _vectorLineP.lock())
        return vectorLineP->owner;
    return nullptr;
}

bool OsmAnd::VectorLine::SymbolsGroup::updatesPresent()
{
    if (const auto vectorLineP = _vectorLineP.lock())
        return vectorLineP->hasUnappliedChanges();

    return false;
}

bool OsmAnd::VectorLine::SymbolsGroup::supportsResourcesRenew()
{
    return true;
}

OsmAnd::IUpdatableMapSymbolsGroup::UpdateResult OsmAnd::VectorLine::SymbolsGroup::update(const MapState& mapState)
{
    UpdateResult result = UpdateResult::None;
    if (const auto vectorLineP = _vectorLineP.lock())
    {
        vectorLineP->update(mapState);
        
        bool hasPropertiesChanges = vectorLineP->hasUnappliedChanges();
        bool hasPrimitiveChanges = vectorLineP->hasUnappliedPrimitiveChanges();
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
        
        vectorLineP->applyChanges();
    }

    return result;
}

OsmAnd::VectorLine::OnPathSymbolData::OnPathSymbolData(
    OsmAnd::PointI position31, float distance, float direction, float elevation, float elevationScaleFactor)
    : position31(position31)
    , distance(distance)
    , direction(direction)
    , elevation(elevation)
    , elevationScaleFactor(elevationScaleFactor)
{
}

OsmAnd::VectorLine::OnPathSymbolData::~OnPathSymbolData()
{
}
