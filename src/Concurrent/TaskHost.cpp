#include "TaskHost.h"

#include "HostedTask.h"

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
