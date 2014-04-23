#include "AccessLockCounter.h"
#include "AccessLockCounter_P.h"

OsmAnd::AccessLockCounter::AccessLockCounter()
    : _p(new AccessLockCounter_P(this))
{
}

OsmAnd::AccessLockCounter::~AccessLockCounter()
{
}

bool OsmAnd::AccessLockCounter::tryLockForReading() const
{
    return _p->tryLockForReading();
}

void OsmAnd::AccessLockCounter::lockForReading() const
{
    _p->lockForReading();
}

void OsmAnd::AccessLockCounter::unlockFromReading() const
{
    _p->unlockFromReading();
}

bool OsmAnd::AccessLockCounter::tryLockForWriting() const
{
    return _p->tryLockForWriting();
}

void OsmAnd::AccessLockCounter::lockForWriting() const
{
    _p->lockForWriting();
}

void OsmAnd::AccessLockCounter::unlockFromWriting() const
{
    _p->unlockFromWriting();
}
