#include "AccessLockCounter.h"
#include "AccessLockCounter_P.h"

OsmAnd::AccessLockCounter::AccessLockCounter()
    : _p(new AccessLockCounter_P(this))
{
}

OsmAnd::AccessLockCounter::~AccessLockCounter()
{
    _p->notifyAboutDestruction();
}

bool OsmAnd::AccessLockCounter::tryLockForReading() const
{
    return _p->tryLockForReading();
}

bool OsmAnd::AccessLockCounter::lockForReading() const
{
    return _p->lockForReading();
}

void OsmAnd::AccessLockCounter::unlockFromReading() const
{
    _p->unlockFromReading();
}

bool OsmAnd::AccessLockCounter::tryLockForWriting() const
{
    return _p->tryLockForWriting();
}

bool OsmAnd::AccessLockCounter::lockForWriting() const
{
    return _p->lockForWriting();
}

void OsmAnd::AccessLockCounter::unlockFromWriting() const
{
    _p->unlockFromWriting();
}
