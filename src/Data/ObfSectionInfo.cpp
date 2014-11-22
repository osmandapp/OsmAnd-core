#include "ObfSectionInfo.h"

#include <cassert>

QAtomicInt OsmAnd::ObfSectionInfo::_nextRuntimeGeneratedId(1);

OsmAnd::ObfSectionInfo::ObfSectionInfo( const std::weak_ptr<ObfInfo>& owner_ )
    : name(_name)
    , length(_length)
    , offset(_offset)
    , owner(owner_)
    , runtimeGeneratedId(_nextRuntimeGeneratedId.fetchAndAddOrdered(1))
{
    // This odd code checks if _nextRuntimeGeneratedId was initialized prior to call of this ctor.
    // This may happen if this ctor is called by initialization of other static variable. And
    // since their initialization order is not defined, this may lead to very odd bugs.
    assert(_nextRuntimeGeneratedId.loadAcquire() >= 2);
}

OsmAnd::ObfSectionInfo::~ObfSectionInfo()
{
}
