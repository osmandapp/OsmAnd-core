#include "Task.h"

OsmAnd::Concurrent::Task::Task(
    const ExecuteSignature executeFunctor_,
    const PreExecuteSignature preExecuteFunctor_ /*= nullptr*/,
    const PostExecuteSignature postExecuteFunctor_ /*= nullptr*/)
    : _cancellationRequestedByTask(false)
    , _cancellationRequestedByExternal(0)
    , _cancellator(new Cancellator(this))
    , preExecuteFunctor(preExecuteFunctor_)
    , executeFunctor(executeFunctor_)
    , postExecuteFunctor(postExecuteFunctor_)
{
    assert(executeFunctor != nullptr);
}

OsmAnd::Concurrent::Task::~Task()
{
    _cancellator->unlink();
}

void OsmAnd::Concurrent::Task::requestCancellation()
{
    _cancellationRequestedByExternal.fetchAndAddOrdered(1);
}

bool OsmAnd::Concurrent::Task::isCancellationRequested() const
{
    return _cancellationRequestedByTask || _cancellationRequestedByExternal.loadAcquire() > 0;
}

std::weak_ptr<OsmAnd::Concurrent::Task::Cancellator> OsmAnd::Concurrent::Task::obtainCancellator()
{
    return _cancellator;
}

std::weak_ptr<const OsmAnd::Concurrent::Task::Cancellator> OsmAnd::Concurrent::Task::obtainCancellator() const
{
    return _cancellator;
}

void OsmAnd::Concurrent::Task::run()
{
    // Check if task wants to cancel itself
    if (preExecuteFunctor && _cancellationRequestedByExternal.loadAcquire() == 0)
    {
        bool cancellationRequestedByTask = false;
        preExecuteFunctor(this, cancellationRequestedByTask);
        _cancellationRequestedByTask = cancellationRequestedByTask;
    }

    // If cancellation was not requested by task itself nor by external call
    if (!_cancellationRequestedByTask && _cancellationRequestedByExternal.loadAcquire() == 0)
        executeFunctor(this);

    // Report that execution had finished
    if (postExecuteFunctor)
        postExecuteFunctor(this, isCancellationRequested());
}

OsmAnd::Concurrent::Task::Cancellator::Cancellator(OsmAnd::Concurrent::Task *const taskRef)
    : _taskRef(taskRef)
{
}

OsmAnd::Concurrent::Task::Cancellator::~Cancellator()
{
}

void OsmAnd::Concurrent::Task::Cancellator::requestCancellation(bool* const pOutSuccess /* = nullptr*/)
{
    QReadLocker scopedLock(&_taskRefLock);

    if (!_taskRef)
    {
        if (pOutSuccess)
            *pOutSuccess = false;
        return;
    }

    if (pOutSuccess)
        *pOutSuccess = true;
    _taskRef->requestCancellation();
}

bool OsmAnd::Concurrent::Task::Cancellator::isCancellationRequested(bool* const pOutSuccess /* = nullptr*/) const
{
    QReadLocker scopedLock(&_taskRefLock);

    if (!_taskRef)
    {
        if (pOutSuccess)
            *pOutSuccess = false;

        return false;
    }

    if (pOutSuccess)
        *pOutSuccess = true;
    return _taskRef->isCancellationRequested();
}

bool OsmAnd::Concurrent::Task::Cancellator::isLinked() const
{
    QReadLocker scopedLock(&_taskRefLock);

    return (_taskRef != nullptr);
}

void OsmAnd::Concurrent::Task::Cancellator::unlink()
{
    QWriteLocker scopedLock(&_taskRefLock);

    _taskRef = nullptr;
}
