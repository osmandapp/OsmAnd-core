#include "ObfRoutingSectionInfo_P.h"

OsmAnd::ObfRoutingSectionInfo_P::ObfRoutingSectionInfo_P(ObfRoutingSectionInfo* owner_)
    : _encodingDecodingRules()
#if !defined(ATOMIC_POINTER_LOCK_FREE)
    , _encodingDecodingRulesLoaded(0)
#endif !defined(ATOMIC_POINTER_LOCK_FREE)
    , owner(owner_)
{
}

OsmAnd::ObfRoutingSectionInfo_P::~ObfRoutingSectionInfo_P()
{
}

std::shared_ptr<const OsmAnd::ObfRoutingSectionEncodingDecodingRules> OsmAnd::ObfRoutingSectionInfo_P::getEncodingDecodingRules() const
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

OsmAnd::ObfRoutingSectionLevel_P::ObfRoutingSectionLevel_P(ObfRoutingSectionLevel* const owner_)
    : owner(owner_)
{
}

OsmAnd::ObfRoutingSectionLevel_P::~ObfRoutingSectionLevel_P()
{
}
