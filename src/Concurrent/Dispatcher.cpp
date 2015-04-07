#include "Dispatcher.h"

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
