#include "Concurrent.h"

#include <cassert>

#include "Common.h"

OsmAnd::Concurrent::Task::Task(ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/)
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

OsmAnd::Concurrent::TaskHost::TaskHost(const OwnerPtr& ownerPtr_)
    : _ownerPtr(ownerPtr_)
    , _ownerIsBeingDestructed(false)
{
}

OsmAnd::Concurrent::TaskHost::~TaskHost()
{
}

void OsmAnd::Concurrent::TaskHost::onOwnerIsBeingDestructed()
{
    // Mark that owner is being destructed
    _ownerIsBeingDestructed = true;

    // Ask all tasks to cancel
    {
        QReadLocker scopedLocker(&_hostedTasksLock);

        for (const auto& task : constOf(_hostedTasks))
            task->requestCancellation();
    }

    // Hold until all tasks are released
    {
        QReadLocker scopedLocker(&_hostedTasksLock);
        while (_hostedTasks.size() != 0)
            REPEAT_UNTIL(_unlockedCondition.wait(&_hostedTasksLock));
    }
}

OsmAnd::Concurrent::TaskHost::Bridge::Bridge(const OwnerPtr& owner)
    : _host(new TaskHost(owner))
{
}

OsmAnd::Concurrent::TaskHost::Bridge::~Bridge()
{
    {
        QReadLocker scopedLocker(&_host->_hostedTasksLock);
        assert(_host->_hostedTasks.size() == 0);
    }
}

void OsmAnd::Concurrent::TaskHost::Bridge::onOwnerIsBeingDestructed() const
{
    _host->onOwnerIsBeingDestructed();
}

OsmAnd::Concurrent::HostedTask::HostedTask(const TaskHost::Bridge& bridge, ExecuteSignature executeMethod_, PreExecuteSignature preExecuteMethod_ /*= nullptr*/, PostExecuteSignature postExecuteMethod_ /*= nullptr*/)
    : Task(executeMethod_, preExecuteMethod_, postExecuteMethod_)
    , _host(bridge._host)
    , _lockedOwner(nullptr)
    , lockedOwner(_lockedOwner)
{
    // Ensure that owner is not being destructed
    if (_host->_ownerIsBeingDestructed)
    {
        requestCancellation();
        return;
    }

    // When task is created, during it's lifetime task host must exist
    {
        QWriteLocker scopedLocker(&_host->_hostedTasksLock);
        _host->_hostedTasks.push_back(this);
    }
    _lockedOwner = _host->_ownerPtr;
}

OsmAnd::Concurrent::HostedTask::~HostedTask()
{
    // When hosted task is being destroyed, it unlocks owner
    {
        QWriteLocker scopedLocker(&_host->_hostedTasksLock);
        _host->_hostedTasks.removeOne(this);
    }
    _host->_unlockedCondition.wakeOne();
}

void OsmAnd::Concurrent::HostedTask::run()
{
    // Do nothing if owner is being destructed
    if (_host->_ownerIsBeingDestructed)
        requestCancellation();

    // Execute task itself
    Task::run();
}

OsmAnd::Concurrent::Thread::Thread(ThreadProcedureSignature threadProcedure_)
    : QThread(nullptr)
    , threadProcedure(threadProcedure_)
{
}

OsmAnd::Concurrent::Thread::~Thread()
{
}

void OsmAnd::Concurrent::Thread::run()
{
    assert(threadProcedure);

    // Simply execute procedure
    threadProcedure();
}

OsmAnd::Concurrent::Dispatcher::Dispatcher()
{
}

OsmAnd::Concurrent::Dispatcher::~Dispatcher()
{
}

int OsmAnd::Concurrent::Dispatcher::queueSize() const
{
    QMutexLocker scopedLocker(&_queueMutex);

    return _queue.size();
}

void OsmAnd::Concurrent::Dispatcher::runAll()
{
    for (;;)
    {
        Delegate method = nullptr;
        {
            QMutexLocker scopedLocker(&_queueMutex);

            if (_queue.isEmpty())
                break;
            method = _queue.dequeue();
        }

        method();
    }
}

void OsmAnd::Concurrent::Dispatcher::runOne()
{
    Delegate method = nullptr;
    {
        QMutexLocker scopedLocker(&_queueMutex);

        if (_queue.isEmpty())
            return;
        method = _queue.dequeue();
    }

    method();
}

void OsmAnd::Concurrent::Dispatcher::run()
{
    _shutdownRequested = false;
    _isRunningStandalone = true;

    while (!_shutdownRequested)
        runOne();

    _isRunningStandalone = false;
    {
        QMutexLocker scopedLocker(&_performedShutdownConditionMutex);
        _performedShutdown.wakeAll();
    }
}

void OsmAnd::Concurrent::Dispatcher::invoke(const Delegate method)
{
    assert(method != nullptr);

    QMutex waitMutex;
    QWaitCondition waitCondition;
    invokeAsync([&waitCondition, &waitMutex, method]
    {
        method();

        {
            QMutexLocker scopedLocker(&waitMutex);
            waitCondition.wakeAll();
        }
    });

    {
        QMutexLocker scopedLocker(&waitMutex);
        REPEAT_UNTIL(waitCondition.wait(&waitMutex));
    }
}

void OsmAnd::Concurrent::Dispatcher::invokeAsync(const Delegate method)
{
    QMutexLocker scopedLocker(&_queueMutex);

    _queue.enqueue(method);
}

void OsmAnd::Concurrent::Dispatcher::shutdown()
{
    QMutexLocker scopedLocker(&_performedShutdownConditionMutex);

    shutdownAsync();

    REPEAT_UNTIL(_performedShutdown.wait(&_performedShutdownConditionMutex));
}

void OsmAnd::Concurrent::Dispatcher::shutdownAsync()
{
    assert(_isRunningStandalone);

    _shutdownRequested = true;
}
