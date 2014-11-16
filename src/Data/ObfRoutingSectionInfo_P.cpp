#include "ObfRoutingSectionInfo_P.h"

OsmAnd::ObfRoutingSectionInfo_P::ObfRoutingSectionInfo_P(ObfRoutingSectionInfo* owner_)
    : _encodingDecodingRules()
    , _encodingDecodingRulesLoaded(0)
    , owner(owner_)
{
}

OsmAnd::ObfRoutingSectionInfo_P::~ObfRoutingSectionInfo_P()
{
}

std::shared_ptr<const OsmAnd::ObfRoutingSectionEncodingDecodingRules> OsmAnd::ObfRoutingSectionInfo_P::getEncodingDecodingRules() const
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

OsmAnd::ObfRoutingSectionLevel_P::ObfRoutingSectionLevel_P(ObfRoutingSectionLevel* const owner_)
    : owner(owner_)
{
}

OsmAnd::ObfRoutingSectionLevel_P::~ObfRoutingSectionLevel_P()
{
}
