#include "Dispatcher.h"

#include <cassert>

int OsmAnd::Concurrent::Dispatcher::queueSize() const
{
    std::lock_guard<std::mutex> scopedLocker(_queueMutex);
    return _queue.size();
}

void OsmAnd::Concurrent::Dispatcher::runAll()
{
    while (runOne())
    {
    };
}

bool OsmAnd::Concurrent::Dispatcher::runOne()
{
    Delegate method = nullptr;
    {
        std::lock_guard<std::mutex> scopedLocker(_queueMutex);
        if (_queue.empty())
        {
            return false;
        }
        method = _queue.front();
        _queue.pop();
    }

    method();
    return true;
}

void OsmAnd::Concurrent::Dispatcher::run()
{
    {
        std::lock_guard<std::mutex> _scopedLocker(_runnerMutex);
        _shutdownRequested = false;
        _isRunningStandalone = true;
    }
    bool keepRunning = true;
    while (keepRunning)
    {
        // WARNING: if _queue is empty, this will become a tight loop
        //TODO: wait on _queueCondition
        bool workDone = runOne();

        std::lock_guard<std::mutex> _scopedLocker(_runnerMutex);
        keepRunning = !_shutdownRequested;
    }

    std::lock_guard<std::mutex> _scopedLocker(_runnerMutex);
    _isRunningStandalone = false;
    _runnerCondition.notify_all();
}

void OsmAnd::Concurrent::Dispatcher::invoke(const Delegate method)
{
    assert(method != nullptr);

    std::mutex waitMutex;
    std::condition_variable waitCondition;
    bool done = false;
    invokeAsync(
        [&waitCondition, &waitMutex, method = std::move( method ), &done]
        {
            method();
            std::lock_guard<std::mutex> scopedLocker(waitMutex);
            done = true;
            waitCondition.notify_all();
        });

    std::unique_lock<std::mutex> scopedLocker(waitMutex);
    waitCondition.wait(
        scopedLocker,
        [&done]()
        {
            return done;
        });
}

void OsmAnd::Concurrent::Dispatcher::invokeAsync(const Delegate method)
{
    std::lock_guard<std::mutex> scopedLocker(_queueMutex);
    _queue.push(method);
}

void OsmAnd::Concurrent::Dispatcher::shutdown()
{
    shutdownAsync();
    std::unique_lock<std::mutex> scopedLocker(_runnerMutex);
    _runnerCondition.wait(
        scopedLocker,
        [this]()
        {
            return !_isRunningStandalone;
        });
}

void OsmAnd::Concurrent::Dispatcher::shutdownAsync()
{
    std::lock_guard<std::mutex> scopedLocker(_runnerMutex);
    assert(_isRunningStandalone);
    _shutdownRequested = true;
}
