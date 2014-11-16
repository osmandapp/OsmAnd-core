#include "ObfMapSectionInfo_P.h"
#include "ObfMapSectionInfo.h"

OsmAnd::ObfMapSectionInfo_P::ObfMapSectionInfo_P(ObfMapSectionInfo* owner_)
    : _encodingDecodingRules()
    , _encodingDecodingRulesLoaded(0)
    , owner(owner_)
{
}

OsmAnd::ObfMapSectionInfo_P::~ObfMapSectionInfo_P()
{
}

std::shared_ptr<const OsmAnd::ObfMapSectionDecodingEncodingRules> OsmAnd::ObfMapSectionInfo_P::getEncodingDecodingRules() const
{
    if (_encodingDecodingRulesLoaded.loadAcquire() != 0)
    {
        if (!_encodingDecodingRules)
        {
            QMutexLocker scopedLocker(&_encodingDecodingRulesLoadMutex);
            return _encodingDecodingRules;
        }
        return _encodingDecodingRules;
    }
    return nullptr;
}

OsmAnd::ObfMapSectionLevel_P::ObfMapSectionLevel_P(ObfMapSectionLevel* owner_)
    : _rootNodes()
    , _rootNodesLoaded(0)
    , owner(owner_)
{
}

OsmAnd::ObfMapSectionLevel_P::~ObfMapSectionLevel_P()
{
}

OsmAnd::ObfMapSectionLevelTreeNode::ObfMapSectionLevelTreeNode(const std::shared_ptr<const ObfMapSectionLevel>& level_)
    : level(level_)
    , dataOffset(0)
    , surfaceType(MapSurfaceType::Undefined)
    , hasChildrenDataBoxes(false)
    , firstDataBoxInnerOffset(0)
{
}

OsmAnd::ObfMapSectionLevelTreeNode::~ObfMapSectionLevelTreeNode()
{
}
