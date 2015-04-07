#include "Task.h"

#include <cassert>

#include "Common.h"

OsmAnd::Concurrent::Task::Task(
    const ExecuteSignature executeFunctor_,
    const PreExecuteSignature preExecuteFunctor_ /*= nullptr*/,
    const PostExecuteSignature postExecuteFunctor_ /*= nullptr*/)
    : _cancellationRequestedByTask(false)
    , _cancellationRequestedByExternal(0)
    , preExecuteFunctor(preExecuteFunctor_)
    , executeFunctor(executeFunctor_)
    , postExecuteFunctor(postExecuteFunctor_)
{
    assert(executeFunctor != nullptr);
}

OsmAnd::Concurrent::Task::~Task()
{
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

    // If cancellation was not requested by task itself nor by
    // external call
    if (!_cancellationRequestedByTask && _cancellationRequestedByExternal.loadAcquire() == 0)
        executeFunctor(this);

    // Report that execution had finished
    if (postExecuteFunctor)
        postExecuteFunctor(this, isCancellationRequested());
}

void OsmAnd::Concurrent::Task::requestCancellation()
{
    _cancellationRequestedByExternal.fetchAndAddOrdered(1);
}

bool OsmAnd::Concurrent::Task::isCancellationRequested() const
{
    return _cancellationRequestedByTask || _cancellationRequestedByExternal.loadAcquire() > 0;
}
