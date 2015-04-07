#include "HostedTask.h"

OsmAnd::Concurrent::HostedTask::HostedTask(
    const TaskHost::Bridge& bridge,
    ExecuteSignature executeFunctor_,
    PreExecuteSignature preExecuteFunctor_ /*= nullptr*/,
    PostExecuteSignature postExecuteFunctor_ /*= nullptr*/)
    : Task(executeFunctor_, preExecuteFunctor_, postExecuteFunctor_)
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
