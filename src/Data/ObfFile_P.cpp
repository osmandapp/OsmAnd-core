#include "ObfFile_P.h"

OsmAnd::ObfFile_P::ObfFile_P(ObfFile* owner_)
    : owner(owner_)
    , _isLockedForRemoval(false)
{
}

OsmAnd::ObfFile_P::~ObfFile_P()
{
    if(_isLockedForRemoval)
        _fileLock.unlock();
}

void OsmAnd::ObfFile_P::lockForRemoval() const
{
    if(_isLockedForRemoval)
        return;
    _fileLock.lockForWrite();
    _isLockedForRemoval = true;
}
