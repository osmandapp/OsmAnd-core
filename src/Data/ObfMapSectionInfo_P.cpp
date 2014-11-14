#include "ObfMapSectionInfo_P.h"
#include "ObfMapSectionInfo.h"

OsmAnd::ObfMapSectionInfo_P::ObfMapSectionInfo_P(ObfMapSectionInfo* owner_)
    : _encodingDecodingRules()
#if !defined(ATOMIC_POINTER_LOCK_FREE)
    , _encodingDecodingRulesLoaded(0)
#endif // !defined(ATOMIC_POINTER_LOCK_FREE)
    , owner(owner_)
{
}

OsmAnd::ObfMapSectionInfo_P::~ObfMapSectionInfo_P()
{
}

std::shared_ptr<const OsmAnd::ObfMapSectionDecodingEncodingRules> OsmAnd::ObfMapSectionInfo_P::getEncodingDecodingRules() const
{
#if defined(ATOMIC_POINTER_LOCK_FREE)
    return std::atomic_load(&_encodingDecodingRules);
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
    if (_encodingDecodingRulesLoaded.loadAcquire() != 0)
    {
        if (!_encodingDecodingRules)
        {
            QMutexLocker scopedLocker(&section->_p->_encodingDecodingRulesLoadMutex);
            return _encodingDecodingRules;
        }
        return _encodingDecodingRules;
    }
    return nullptr;
#endif
}

OsmAnd::ObfMapSectionLevel_P::ObfMapSectionLevel_P(ObfMapSectionLevel* owner_)
    : _rootNodes()
#if !defined(ATOMIC_POINTER_LOCK_FREE)
    , _rootNodesLoaded(0)
#endif // !defined(ATOMIC_POINTER_LOCK_FREE)
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
