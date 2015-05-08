#include "ObfRoutingSectionInfo_P.h"

OsmAnd::ObfRoutingSectionInfo_P::ObfRoutingSectionInfo_P(ObfRoutingSectionInfo* owner_)
    : _attributeMapping()
    , _attributeMappingLoaded(0)
    , owner(owner_)
{
}

OsmAnd::ObfRoutingSectionInfo_P::~ObfRoutingSectionInfo_P()
{
}

std::shared_ptr<const OsmAnd::ObfRoutingSectionAttributeMapping> OsmAnd::ObfRoutingSectionInfo_P::getAttributeMapping() const
{
    if (_attributeMappingLoaded.loadAcquire() != 0)
    {
        if (!_attributeMapping)
        {
            QMutexLocker scopedLocker(&_attributeMappingLoadMutex);
            return _attributeMapping;
        }
        return _attributeMapping;
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
