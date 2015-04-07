#include "Task.h"

#include <cassert>

#include "Common.h"

OsmAnd::Concurrent::Task::Task(
    ExecuteSignature executeMethod,
    PreExecuteSignature preExecuteMethod /*= nullptr*/,
    PostExecuteSignature postExecuteMethod /*= nullptr*/)
    : _cancellationRequestedByTask(false)
    , _cancellationRequestedByExternal(0)
    , preExecute(preExecuteMethod)
    , execute(executeMethod)
    , postExecute(postExecuteMethod)
{
    assert(executeMethod != nullptr);
}

OsmAnd::Concurrent::Task::~Task()
{
}

void OsmAnd::Concurrent::Task::run()
{
    // Check if task wants to cancel itself
    if (preExecute && _cancellationRequestedByExternal.loadAcquire() == 0)
    {
        bool cancellationRequestedByTask = false;
        preExecute(this, cancellationRequestedByTask);
        _cancellationRequestedByTask = cancellationRequestedByTask;
    }

    // If cancellation was not requested by task itself nor by
    // external call
    if (!_cancellationRequestedByTask && _cancellationRequestedByExternal.loadAcquire() == 0)
        execute(this);

    // Report that execution had finished
    if (postExecute)
        postExecute(this, isCancellationRequested());
}

void OsmAnd::Concurrent::Task::requestCancellation()
{
    _cancellationRequestedByExternal.fetchAndAddOrdered(1);
}

bool OsmAnd::Concurrent::Task::isCancellationRequested() const
{
    return _cancellationRequestedByTask || _cancellationRequestedByExternal.loadAcquire() > 0;
}
