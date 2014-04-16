#include "ObfFile_P.h"

OsmAnd::ObfFile_P::ObfFile_P(ObfFile* owner_)
    : owner(owner_)
{
}

OsmAnd::ObfFile_P::~ObfFile_P()
{
}

bool OsmAnd::ObfFile_P::tryLockForReading() const
{
    return true;
}

void OsmAnd::ObfFile_P::lockForReading() const
{
    
}

void OsmAnd::ObfFile_P::unlockFromReading() const
{
}

bool OsmAnd::ObfFile_P::tryLockForWriting() const
{
    return true;
}

void OsmAnd::ObfFile_P::lockForWriting() const
{
}

void OsmAnd::ObfFile_P::unlockFromWriting() const
{
}
