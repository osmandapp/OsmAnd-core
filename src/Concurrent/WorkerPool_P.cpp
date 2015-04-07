#include "WorkerPool_P.h"
#include "WorkerPool.h"

#include "QtExtensions.h"
#include <QElapsedTimer>

#include "Logging.h"

OsmAnd::Concurrent::WorkerPool_P::WorkerPool_P(WorkerPool* const owner_, const Order order_, const int maxThreadCount_)
    : _order(static_cast<int>(order_))
    , _maxThreadCount(maxThreadCount_)
    , _isBeingReset(false)
    , owner(owner_)
{
}

OsmAnd::Concurrent::WorkerPool_P::~WorkerPool_P()
{
}

OsmAnd::Concurrent::WorkerPool_P::Order OsmAnd::Concurrent::WorkerPool_P::order() const
{
    return static_cast<Order>(_order.loadAcquire());
}

void OsmAnd::Concurrent::WorkerPool_P::setOrder(const Order order)
{
    _order.storeRelease(static_cast<int>(order));
}

int OsmAnd::Concurrent::WorkerPool_P::maxThreadCount() const
{
    return _maxThreadCount.loadAcquire();
}

void OsmAnd::Concurrent::WorkerPool_P::setMaxThreadCount(int maxThreadCount)
{
    const auto oldMaxThreadCount = _maxThreadCount.fetchAndStoreOrdered(maxThreadCount);
    if (maxThreadCount > 0 && oldMaxThreadCount <= maxThreadCount)
        return;

    QMutexLocker scopedLocker(&_mutex);
    tryLaunchNextRunnables();
}

unsigned int OsmAnd::Concurrent::WorkerPool_P::activeThreadCount() const
{
    QMutexLocker scopedLocker(&_mutex);

    return activeThreadCountNoLock();
}

bool OsmAnd::Concurrent::WorkerPool_P::waitForDone(const int msecs) const
{
    QMutexLocker scopedLocker(&_mutex);
    
    return waitForDoneNoLock(msecs);
}

void OsmAnd::Concurrent::WorkerPool_P::enqueue(QRunnable* const runnable)
{
    QMutexLocker scopedLocker(&_mutex);
    
    _queue.push_front(runnable);
    tryLaunchNextRunnable();
}

bool OsmAnd::Concurrent::WorkerPool_P::dequeue(QRunnable* const runnable)
{
    QMutexLocker scopedLocker(&_mutex);
    
    return _queue.removeOne(runnable);
}

void OsmAnd::Concurrent::WorkerPool_P::dequeueAll()
{
    QMutexLocker scopedLocker(&_mutex);

    dequeueAllNoLock();
}

void OsmAnd::Concurrent::WorkerPool_P::reset()
{
    QMutexLocker scopedLocker(&_mutex);

    dequeueAllNoLock();
    _isBeingReset = true;
    REPEAT_UNTIL(waitForDoneNoLock(-1));
    _isBeingReset = false;
}

unsigned int OsmAnd::Concurrent::WorkerPool_P::activeThreadCountNoLock() const
{
    return _allThreads.count() - _freeThreads.count() - _inactiveThreads.count();
}

void OsmAnd::Concurrent::WorkerPool_P::createNewThread()
{
    const auto thread = new WorkerThread(this);

    thread->setObjectName(QLatin1String("Worker (pooled)"));
    _allThreads.insert(thread);
    _freeThreads.enqueue(thread);

    thread->start();
}

bool OsmAnd::Concurrent::WorkerPool_P::tryLaunchNextRunnable()
{
    // In case there are no threads, create new one
    if (_allThreads.isEmpty())
    {
        createNewThread();
        return true;
    }

    // Check if max threads limit is not reached
    if (tooManyThreadsActive())
        return false;

    // If free thread is available, use it
    if (!_freeThreads.isEmpty())
    {
        _freeThreads.head()->wakeup.wakeOne();
        return true;
    }

    // If inactive thread is available, use it
    if (!_inactiveThreads.isEmpty())
    {
        _inactiveThreads.head()->wakeup.wakeOne();
        return true;
    }

    // Otherwise, just start new thread
    createNewThread();
    return true;
}

void OsmAnd::Concurrent::WorkerPool_P::tryLaunchNextRunnables()
{
    while (!_queue.isEmpty() && tryLaunchNextRunnable());
}

QRunnable* OsmAnd::Concurrent::WorkerPool_P::takeNextRunnable()
{
    if (_queue.isEmpty())
        return nullptr;

    switch (order())
    {
        case Order::FIFO:
            return _queue.takeLast();
        case Order::LIFO:
            return _queue.takeFirst();
        case Order::Random:
            return _queue.takeAt(qrand() % _queue.size());
        default:
            return nullptr;
    }
}

bool OsmAnd::Concurrent::WorkerPool_P::tooManyThreadsActive() const
{
    const auto maxThreadCount = this->maxThreadCount();
    return maxThreadCount > 0 && activeThreadCountNoLock() >= maxThreadCount;
}

void OsmAnd::Concurrent::WorkerPool_P::dequeueAllNoLock()
{
    for (auto runnable : _queue)
    {
        if (runnable->autoDelete())
            delete runnable;
    }
    _queue.clear();
}

bool OsmAnd::Concurrent::WorkerPool_P::waitForDoneNoLock(const int msecs) const
{
    if (msecs < 0)
    {
        while (!_allThreads.isEmpty() || !_queue.isEmpty())
            REPEAT_UNTIL(_threadExited.wait(&_mutex));
    }
    else
    {
        QElapsedTimer waitTimer;
        waitTimer.start();
        int timeLeft;
        while ((!_allThreads.isEmpty() || !_queue.isEmpty()) && ((timeLeft = msecs - waitTimer.elapsed()) > 0))
            _threadExited.wait(&_mutex, timeLeft);
    }

    return _queue.isEmpty() && _allThreads.isEmpty();
}

OsmAnd::Concurrent::WorkerPool_P::WorkerThread::WorkerThread(WorkerPool_P* const pool_)
    : pool(pool_)
{
}

OsmAnd::Concurrent::WorkerPool_P::WorkerThread::~WorkerThread()
{
}

void OsmAnd::Concurrent::WorkerPool_P::WorkerThread::run()
{
    for (;;)
    {
        // Get the runnable
        QRunnable* runnable = nullptr;
        {
            QMutexLocker scopedLocker(&pool->_mutex);

            // In case everything is being reset, self-destroy
            if (pool->_isBeingReset)
            {
                pool->_freeThreads.removeOne(this);
                pool->_allThreads.remove(this);
                return;
            }

            // Check if not out of limit or sleep
            if (pool->tooManyThreadsActive())
            {
                pool->_freeThreads.removeOne(this);
                pool->_inactiveThreads.enqueue(this);

                REPEAT_UNTIL(wakeup.wait(&pool->_mutex));

                pool->_inactiveThreads.removeOne(this);
                pool->_freeThreads.enqueue(this);
                continue;
            }

            // Take runnable or sleep
            runnable = pool->takeNextRunnable();
            if (!runnable)
            {
                REPEAT_UNTIL(wakeup.wait(&pool->_mutex));
                continue;
            }

            // Since runnable was obtained, this thread is no longer free
            pool->_freeThreads.removeOne(this);
        }
        
        // Execute the runnable
#ifndef QT_NO_EXCEPTIONS
        try
        {
#endif
            runnable->run();
#ifndef QT_NO_EXCEPTIONS

            if (runnable->autoDelete())
                delete runnable;
            runnable = nullptr;
        }
        catch (...)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Exception was caught during execution of worker runnable %p. It must be caught inside runnable, otherwise runnable will be lost!", runnable);
        }
#endif

        // After runnable execution is complete, free this thread
        {
            QMutexLocker scopedLocker(&pool->_mutex);

            pool->_freeThreads.enqueue(this);
            pool->_threadFreed.wakeOne();
        }
    }
}
