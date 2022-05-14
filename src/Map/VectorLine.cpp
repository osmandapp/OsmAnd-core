#include "VectorLine.h"
#include "VectorLine_P.h"

OsmAnd::VectorLine::VectorLine(
    const int lineId_,
    const int baseOrder_,
    const sk_sp<const SkImage>& pathIcon_/* = nullptr*/,
    const sk_sp<const SkImage>& specialPathIcon_/* = nullptr*/,
    const float pathIconStep_/* = -1*/,
    const bool pathIconOnSurface_/* = true*/,
    const float screenScale_/* = 2*/,
    const VectorLine::EndCapStyle endCapStlye_/* = LineEndCapStyle::ROUND*/)
    : _p(new VectorLine_P(this))
    , lineId(lineId_)
    , baseOrder(baseOrder_)
    , pathIcon(pathIcon_)
    , specialPathIcon(specialPathIcon_)
    , pathIconStep(pathIconStep_)
    , pathIconOnSurface(pathIconOnSurface_)
    , screenScale(screenScale_)
    , endCapStyle(endCapStlye_)
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

void OsmAnd::VectorLine::setPoints(const QVector<OsmAnd::PointI>& points)
{
    _p->setPoints(points);    
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLine::getColorizationMapping() const
{
    return _p->getColorizationMapping();
}

void OsmAnd::VectorLine::setColorizationMapping(const QList<FColorARGB> &colorizationMapping)
{
    _p->setColorizationMapping(colorizationMapping);
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

OsmAnd::VectorLine::OnPathSymbolData::OnPathSymbolData(OsmAnd::PointI position31, float direction)
    : position31(position31)
    , direction(direction)
{
}

OsmAnd::VectorLine::OnPathSymbolData::~OnPathSymbolData()
{
}
