#include "VectorLine.h"
#include "VectorLine_P.h"

OsmAnd::VectorLine::VectorLine(
    const int lineId_,
    const int baseOrder_,
    const double lineWidth_,
    const std::shared_ptr<const SkBitmap>& pathIcon_/* = nullptr*/,
    const float pathIconStep_/* = -1*/)
    : _p(new VectorLine_P(this))
    , lineId(lineId_)
    , baseOrder(baseOrder_)
    , lineWidth(lineWidth_)
    , pathIcon(pathIcon_)
    , pathIconStep(pathIconStep_)
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

QVector<OsmAnd::PointI> OsmAnd::VectorLine::getPoints() const
{
    return _p->getPoints();
}

void OsmAnd::VectorLine::setPoints(const QVector<OsmAnd::PointI>& points)
{
    _p->setPoints(points);    
}

void OsmAnd::VectorLine::setFillColor(const FColorARGB color)
{
    _p->setFillColor(color);
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
