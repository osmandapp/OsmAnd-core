#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"

OsmAnd::ObfRoutingSectionInfo::ObfRoutingSectionInfo(const std::weak_ptr<ObfInfo>& owner)
    : ObfSectionInfo(owner)
    , _p(new ObfRoutingSectionInfo_P(this))
    , decodingRules(_p->_decodingRules)
{
}

OsmAnd::ObfRoutingSectionInfo::~ObfRoutingSectionInfo()
{
}

OsmAnd::ObfRoutingSectionLevel::ObfRoutingSectionLevel(const RoutingDataLevel dataLevel_)
    : _p(new ObfRoutingSectionLevel_P(this))
    , dataLevel(dataLevel_)
    , rootNodes(_p->_rootNodes)
{
}

OsmAnd::ObfRoutingSectionLevel::~ObfRoutingSectionLevel()
{
}

OsmAnd::ObfRoutingSectionLevelTreeNode::ObfRoutingSectionLevelTreeNode()
    : length(0)
    , offset(0)
    , childrenRelativeOffset(0)
    , dataOffset(0)
{
}

OsmAnd::ObfRoutingSectionLevelTreeNode::~ObfRoutingSectionLevelTreeNode()
{
}
