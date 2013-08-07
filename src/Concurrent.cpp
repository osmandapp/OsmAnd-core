#include "Concurrent.h"

#include <assert.h>

const std::shared_ptr<OsmAnd::Concurrent::Pools> OsmAnd::Concurrent::Pools::instance(new OsmAnd::Concurrent::Pools());
const std::shared_ptr<OsmAnd::Concurrent::Pools> OsmAnd::Concurrent::pools(OsmAnd::Concurrent::Pools::instance);

OsmAnd::Concurrent::Pools::Pools()
    : localStorage(new QThreadPool())
    , network(new QThreadPool())
{
    localStorage->setMaxThreadCount(1);
    network->setMaxThreadCount(1);
}

OsmAnd::Concurrent::Pools::~Pools()
{
}

OsmAnd::Concurrent::Task::Task( ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/ )
    : _isCancellationRequested(false)
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
    // This local event loop
    QEventLoop localLoop;

    // If already requested for cancellation, do nothing
    if(_isCancellationRequested)
        return;

    if(preExecute)
        _isCancellationRequested = preExecute(this);
    if(_isCancellationRequested)
        return;

    execute(this, localLoop);

    if(postExecute)
        postExecute(this);
}

void OsmAnd::Concurrent::Task::requestCancellation()
{
    _isCancellationRequested = true;
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
        _isCancellationRequested = true;
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
        _isCancellationRequested = true;

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
