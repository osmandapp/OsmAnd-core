#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"

OsmAnd::ObfRoutingSectionInfo::ObfRoutingSectionInfo( const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , _p(new ObfRoutingSectionInfo_P(this))
    , subsections(_subsections)
    , baseSubsections(_baseSubsections)
{
}

OsmAnd::ObfRoutingSectionInfo::~ObfRoutingSectionInfo()
{
}

OsmAnd::ObfRoutingSubsectionInfo::ObfRoutingSubsectionInfo(const std::shared_ptr<const ObfRoutingSubsectionInfo>& parent_)
    : ObfSectionInfo(parent_->owner)
    , _dataOffset(0)
    , _subsectionsOffset(0)
    , area31(_area31)
    , parent(parent_)
    , section(parent_->section)
{
}

OsmAnd::ObfRoutingSubsectionInfo::ObfRoutingSubsectionInfo(const std::shared_ptr<const ObfRoutingSectionInfo>& section_)
    : ObfSectionInfo(section_->owner)
    , _dataOffset(0)
    , _subsectionsOffset(0)
    , area31(_area31)
    , section(section_)
{
}

OsmAnd::ObfRoutingSubsectionInfo::~ObfRoutingSubsectionInfo()
{
}

bool OsmAnd::ObfRoutingSubsectionInfo::containsData() const
{
    return (_dataOffset != 0);
}

OsmAnd::ObfRoutingBorderLineHeader::ObfRoutingBorderLineHeader()
    : _x2present(false)
    , _y2present(false)
    , x(_x)
    , y(_y)
    , x2(_x2)
    , x2present(_x2present)
    , y2(_y2)
    , y2present(_y2present)
    , offset(_offset)
{
}

OsmAnd::ObfRoutingBorderLineHeader::~ObfRoutingBorderLineHeader()
{
}

OsmAnd::ObfRoutingBorderLinePoint::ObfRoutingBorderLinePoint()
    : id(_id)
    , location(_location)
    , types(_types)
    , distanceToStartPoint(_distanceToStartPoint)
    , distanceToEndPoint(_distanceToEndPoint)
{
}

OsmAnd::ObfRoutingBorderLinePoint::~ObfRoutingBorderLinePoint()
{
}

std::shared_ptr<OsmAnd::ObfRoutingBorderLinePoint> OsmAnd::ObfRoutingBorderLinePoint::bboxedClone( uint32_t x31 ) const
{
    std::shared_ptr<ObfRoutingBorderLinePoint> clone(new ObfRoutingBorderLinePoint());
    clone->_location = location;
    clone->_location.x = x31;
    clone->_id = std::numeric_limits<uint64_t>::max();
    return clone;
}
