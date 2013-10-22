#include "ObfSectionInfo.h"

QAtomicInt OsmAnd::ObfSectionInfo::_nextRuntimeGeneratedId(1);

OsmAnd::ObfSectionInfo::ObfSectionInfo( const std::weak_ptr<ObfInfo>& owner_ )
    : name(_name)
    , length(_length)
    , offset(_offset)
    , owner(owner_)
    , runtimeGeneratedId(_nextRuntimeGeneratedId.fetchAndAddOrdered(1))
{
}

OsmAnd::ObfSectionInfo::~ObfSectionInfo()
{
}
