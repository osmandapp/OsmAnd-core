#include "TaskHost.h"

#include "HostedTask.h"
#include "Logging.h"

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
    requestCancellation();
    waitUntilAllTasksAreReleased();
}

bool OsmAnd::Concurrent::TaskHost::isOwnerBeingDestructed() const
{
    return (_ownerIsBeingDestructed.loadAcquire() != 0);
}

void OsmAnd::Concurrent::TaskHost::requestCancellation()
{
    // Mark the owner while holding the task list lock, so new tasks can not
    // register between the shutdown check and insertion into _hostedTasks.
    {
        QWriteLocker scopedLocker(&_hostedTasksLock);

        _ownerIsBeingDestructed.storeRelease(1);

        for (const auto& task : constOf(_hostedTasks))
            task->requestCancellation();
    }
}

void OsmAnd::Concurrent::TaskHost::waitUntilAllTasksAreReleased()
{
    // Hold until all tasks are released
    {
        QReadLocker scopedLocker(&_hostedTasksLock);
        auto timeoutCounter = 0u;
        while (_hostedTasks.size() != 0)
        {
            if (!_unlockedCondition.wait(&_hostedTasksLock, 1000))
            {
                timeoutCounter++;
                if (timeoutCounter == 1 || (timeoutCounter % 5) == 0)
                {
                    LogPrintf(LogSeverityLevel::Warning,
                        "TaskHost %p is still waiting for %d hosted task(s) to release",
                        this,
                        _hostedTasks.size());
                }
            }
        }
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

bool OsmAnd::Concurrent::TaskHost::Bridge::isOwnerBeingDestructed() const
{
    return _host->isOwnerBeingDestructed();
}

void OsmAnd::Concurrent::TaskHost::Bridge::requestCancellation() const
{
    _host->requestCancellation();
}

void OsmAnd::Concurrent::TaskHost::Bridge::waitUntilAllTasksAreReleased() const
{
    _host->waitUntilAllTasksAreReleased();
}
