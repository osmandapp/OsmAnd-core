#include "ObfMapSectionInfo_P.h"

OsmAnd::ObfMapSectionInfo_P::ObfMapSectionInfo_P(ObfMapSectionInfo* owner_)
    : owner(owner_)
{
}

OsmAnd::ObfMapSectionInfo_P::~ObfMapSectionInfo_P()
{
}

OsmAnd::ObfMapSectionLevel_P::ObfMapSectionLevel_P(ObfMapSectionLevel* owner_)
    : owner(owner_)
{
}

OsmAnd::ObfMapSectionLevel_P::~ObfMapSectionLevel_P()
{
}

OsmAnd::ObfMapSectionLevelTreeNode::ObfMapSectionLevelTreeNode(const std::shared_ptr<const ObfMapSectionLevel>& level_)
    : _childrenInnerOffset(0)
    , _dataOffset(0)
    , _foundation(MapFoundationType::Undefined)
    , level(level_)
{
}

OsmAnd::ObfMapSectionLevelTreeNode::~ObfMapSectionLevelTreeNode()
{
}
