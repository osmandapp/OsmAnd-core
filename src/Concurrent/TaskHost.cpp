#include "TaskHost.h"

#include "HostedTask.h"

OsmAnd::Concurrent::TaskHost::TaskHost(const OwnerPtr& ownerPtr_)
    : _ownerPtr(ownerPtr_)
    , _ownerIsBeingDestructed(0)
{
}

OsmAnd::Concurrent::TaskHost::~TaskHost()
{
}

void OsmAnd::Concurrent::TaskHost::onOwnerIsBeingDestructed()
{
    QReadLocker scopedLocker(&_hostedTasksLock);

    // Mark that owner is being destructed
    _ownerIsBeingDestructed.storeRelease(1);

    // Ask all tasks to cancel
    for (const auto& task : constOf(_hostedTasks))
        task->requestCancellation();

    // Hold until all tasks are released
    while (_hostedTasks.size() > 0)
        _unlockedCondition.wait(&_hostedTasksLock, 100);
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
