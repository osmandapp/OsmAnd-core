#include "AccessLockCounter_P.h"
#include "AccessLockCounter.h"

OsmAnd::AccessLockCounter_P::AccessLockCounter_P(AccessLockCounter* const owner_)
    : owner(owner_)
    , _lockCounter(0)
{
}

OsmAnd::AccessLockCounter_P::~AccessLockCounter_P()
{
}

bool OsmAnd::AccessLockCounter_P::tryLockForReading() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    if(_lockCounter >= 0)
    {
        _lockCounter++;
        _lockCounterWaitCondition.wakeAll();
        return true;
    }

    return false;
}

void OsmAnd::AccessLockCounter_P::lockForReading() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    while(_lockCounter < 0)
        REPEAT_UNTIL(_lockCounterWaitCondition.wait(&_lockCounterMutex));
    _lockCounter++;
    _lockCounterWaitCondition.wakeAll();
}

void OsmAnd::AccessLockCounter_P::unlockFromReading() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    assert(_lockCounter > 0);
    _lockCounter--;
    _lockCounterWaitCondition.wakeAll();
}

bool OsmAnd::AccessLockCounter_P::tryLockForWriting() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    if(_lockCounter <= 0)
    {
        _lockCounter--;
        _lockCounterWaitCondition.wakeAll();
        return true;
    }

    return false;
}

void OsmAnd::AccessLockCounter_P::lockForWriting() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    while(_lockCounter > 0)
        REPEAT_UNTIL(_lockCounterWaitCondition.wait(&_lockCounterMutex));
    _lockCounter--;
    _lockCounterWaitCondition.wakeAll();
}

void OsmAnd::AccessLockCounter_P::unlockFromWriting() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    assert(_lockCounter < 0);
    _lockCounter++;
    _lockCounterWaitCondition.wakeAll();
}
