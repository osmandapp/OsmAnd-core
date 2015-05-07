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
    _queue.reserve(1024);
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

void OsmAnd::Concurrent::WorkerPool_P::enqueue(QRunnable* const runnable, const SortPredicate predicate)
{
    QMutexLocker scopedLocker(&_mutex);
    
    _queue.push_front(runnable);
    if (predicate)
        sortQueueNoLock(predicate);

    tryLaunchNextRunnable();
}

void OsmAnd::Concurrent::WorkerPool_P::enqueue(const QVector<QRunnable*>& runnables, const SortPredicate predicate)
{
    QMutexLocker scopedLocker(&_mutex);

    for (const auto& runnable : constOf(runnables))
        _queue.push_front(runnable);
    if (predicate)
        sortQueueNoLock(predicate);

    tryLaunchNextRunnable();
}

bool OsmAnd::Concurrent::WorkerPool_P::dequeue(QRunnable* const runnable, const SortPredicate predicate)
{
    QMutexLocker scopedLocker(&_mutex);
    
    const auto result = _queue.removeOne(runnable);
    if (result && predicate)
        sortQueueNoLock(predicate);

    return result;
}

void OsmAnd::Concurrent::WorkerPool_P::dequeueAll()
{
    QMutexLocker scopedLocker(&_mutex);

    dequeueAllNoLock();
}

void OsmAnd::Concurrent::WorkerPool_P::sortQueue(const SortPredicate predicate)
{
    QMutexLocker scopedLocker(&_mutex);

    sortQueueNoLock(predicate);
}

void OsmAnd::Concurrent::WorkerPool_P::reset()
{
    QMutexLocker scopedLocker(&_mutex);

    dequeueAllNoLock();
    REPEAT_UNTIL(waitForDoneNoLock(-1));
    _isBeingReset = true;
    while (!_allThreads.isEmpty())
    {
        const auto thread = _freeThreads.head();
        scopedLocker.unlock();

        thread->wakeup.wakeOne();
        thread->wait();
        delete thread;

        scopedLocker.relock();
    }
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
            return _queue.takeFirst();
        case Order::LIFO:
            return _queue.takeLast();
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
        while (activeThreadCountNoLock() != 0 || !_queue.isEmpty())
            REPEAT_UNTIL(_threadFreed.wait(&_mutex));
    }
    else
    {
        QElapsedTimer waitTimer;
        waitTimer.start();
        int timeLeft;
        while ((activeThreadCountNoLock() != 0 || !_queue.isEmpty()) && ((timeLeft = msecs - waitTimer.elapsed()) > 0))
            _threadFreed.wait(&_mutex, timeLeft);
    }

    return activeThreadCountNoLock() == 0 && _queue.isEmpty();
}

void OsmAnd::Concurrent::WorkerPool_P::sortQueueNoLock(const SortPredicate predicate)
{
    std::sort(_queue, predicate);
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
