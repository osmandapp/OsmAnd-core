#include "ObfRoutingSectionInfo_P.h"

#include "Road.h"

OsmAnd::ObfRoutingSectionInfo_P::ObfRoutingSectionInfo_P( ObfRoutingSectionInfo* owner_ )
    : owner(owner_)
{
}

OsmAnd::ObfRoutingSectionInfo_P::~ObfRoutingSectionInfo_P()
{
}

bool OsmAnd::ObfRoutingSectionInfo_P::EncodingRule::isRoundabout() const
{
    return _type == Roundabout;
}

OsmAnd::Model::RoadDirection OsmAnd::ObfRoutingSectionInfo_P::EncodingRule::getDirection() const
{
    if (_type == OneWay)
        return static_cast<Model::RoadDirection>(_parsedValue.asSignedInt);

    return Model::RoadDirection::TwoWay;
}