#include "ObfFile_P.h"

OsmAnd::ObfFile_P::ObfFile_P(ObfFile* owner_)
    : owner(owner_)
    , _lockCounter(0)
{
}

OsmAnd::ObfFile_P::~ObfFile_P()
{
}

bool OsmAnd::ObfFile_P::tryLockForReading() const
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

void OsmAnd::ObfFile_P::lockForReading() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    while(_lockCounter < 0)
        REPEAT_UNTIL(_lockCounterWaitCondition.wait(&_lockCounterMutex));
    _lockCounter++;
    _lockCounterWaitCondition.wakeAll();
}

void OsmAnd::ObfFile_P::unlockFromReading() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    assert(_lockCounter > 0);
    _lockCounter--;
    _lockCounterWaitCondition.wakeAll();
}

bool OsmAnd::ObfFile_P::tryLockForWriting() const
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

void OsmAnd::ObfFile_P::lockForWriting() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    while(_lockCounter > 0)
        REPEAT_UNTIL(_lockCounterWaitCondition.wait(&_lockCounterMutex));
    _lockCounter--;
    _lockCounterWaitCondition.wakeAll();
}

void OsmAnd::ObfFile_P::unlockFromWriting() const
{
    QMutexLocker scopedLocker(&_lockCounterMutex);

    assert(_lockCounter < 0);
    _lockCounter++;
    _lockCounterWaitCondition.wakeAll();
}
