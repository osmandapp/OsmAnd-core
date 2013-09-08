#include "Concurrent.h"

#include <cassert>

const std::shared_ptr<OsmAnd::Concurrent::Pools> OsmAnd::Concurrent::Pools::instance(new OsmAnd::Concurrent::Pools());
const std::shared_ptr<OsmAnd::Concurrent::Pools> OsmAnd::Concurrent::pools(OsmAnd::Concurrent::Pools::instance);

OsmAnd::Concurrent::Pools::Pools()
    : localStorage(new QThreadPool())
    , network(new QThreadPool())
{
    localStorage->setMaxThreadCount(4);
    network->setMaxThreadCount(4);
}

OsmAnd::Concurrent::Pools::~Pools()
{
}

OsmAnd::Concurrent::Task::Task( ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/ )
    : _cancellationRequestedByTask(false)
    , _cancellationRequestedByExternal(false)
    , _cancellationMutex(QMutex::Recursive)
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
    QMutexLocker scopedLocker(&_cancellationMutex);

    // This local event loop
    QEventLoop localLoop;

    // Check if task wants to cancel itself
    if(preExecute && !_cancellationRequestedByExternal)
        preExecute(this, _cancellationRequestedByTask);

    // If cancellation was not requested by task itself nor by
    // external call
    if(!_cancellationRequestedByTask && !_cancellationRequestedByExternal)
        execute(this, localLoop);

    // Report that execution had finished
    if(postExecute)
        postExecute(this, _cancellationRequestedByTask || _cancellationRequestedByExternal);
}

bool OsmAnd::Concurrent::Task::requestCancellation()
{
    if(!_cancellationMutex.tryLock())
        return false;

    _cancellationRequestedByExternal = true;

    _cancellationMutex.unlock();
    return true;
}

bool OsmAnd::Concurrent::Task::isCancellationRequested() const
{
    return _cancellationRequestedByTask || _cancellationRequestedByExternal;
}

OsmAnd::Concurrent::TaskHost::TaskHost( const OwnerPtr& owner )
    : _ownerPtr(owner)
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
        QReadLocker scopedLock(&_hostedTasksLock);

        for(auto itTask = _hostedTasks.begin(); itTask != _hostedTasks.end(); itTask++)
        {
            const auto& task = *itTask;
            task->requestCancellation();
        }
    }

    // Hold until all tasks are released
    for(;;)
    {
        _hostedTasksLock.lockForRead();
        if(_hostedTasks.size() == 0)
        {
            _hostedTasksLock.unlock();
            break;
        }
        _unlockedCondition.wait(&_hostedTasksLock);
        _hostedTasksLock.unlock();
    }
}

OsmAnd::Concurrent::TaskHost::Bridge::Bridge( const OwnerPtr& owner )
    : _host(new TaskHost(owner))
{
}

OsmAnd::Concurrent::TaskHost::Bridge::~Bridge()
{
    {
        QReadLocker scopedLock(&_host->_hostedTasksLock);
        assert(_host->_hostedTasks.size() == 0);
    }
}

void OsmAnd::Concurrent::TaskHost::Bridge::onOwnerIsBeingDestructed() const
{
    _host->onOwnerIsBeingDestructed();
}

OsmAnd::Concurrent::HostedTask::HostedTask( const TaskHost::Bridge& bridge, ExecuteSignature executeMethod_, PreExecuteSignature preExecuteMethod_ /*= nullptr*/, PostExecuteSignature postExecuteMethod_ /*= nullptr*/ )
    : Task(executeMethod_, preExecuteMethod_, postExecuteMethod_)
    , _host(bridge._host)
    , _lockedOwner(nullptr)
    , lockedOwner(_lockedOwner)
{
    // Ensure that owner is not being destructed
    if(_host->_ownerIsBeingDestructed)
    {
        requestCancellation();
        return;
    }

    // When task is created, during it's lifetime task host must exist
    {
        QWriteLocker scopedLock(&_host->_hostedTasksLock);
        _host->_hostedTasks.push_back(this);
    }
    _lockedOwner = _host->_ownerPtr;
}

OsmAnd::Concurrent::HostedTask::~HostedTask()
{
    // When hosted task is being destroyed, it unlocks owner
    {
        QWriteLocker scopedLock(&_host->_hostedTasksLock);
        _host->_hostedTasks.removeOne(this);
    }
    _host->_unlockedCondition.wakeOne();
}

void OsmAnd::Concurrent::HostedTask::run()
{
    // Do nothing if owner is being destructed
    if(_host->_ownerIsBeingDestructed)
        requestCancellation();

    // Execute task itself
    Task::run();
}

OsmAnd::Concurrent::Thread::Thread( ThreadProcedureSignature threadProcedure_ )
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
