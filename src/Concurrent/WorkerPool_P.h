#ifndef _OSMAND_CORE_CONCURRENT_WORKER_POOL_P_H_
#define _OSMAND_CORE_CONCURRENT_WORKER_POOL_P_H_


#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QVector>
#include <QAtomicInt>
#include <QWaitCondition>
#include <QMutex>
#include <QThread>
#include <QSet>
#include <QQueue>
#include "restore_internal_warnings.h"

#include "stdlib_common.h"
#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "WorkerPool.h"

namespace OsmAnd
{
    namespace Concurrent
    {
        class WorkerPool_P Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(WorkerPool_P);

        public:
            typedef WorkerPool::Order Order;
            typedef WorkerPool::SortPredicate SortPredicate;

        private:
            class WorkerThread Q_DECL_FINAL : public QThread
            {
                Q_DISABLE_COPY_AND_MOVE(WorkerThread);

            private:
            protected:
                WorkerThread(WorkerPool_P* const pool);
            public:
                virtual ~WorkerThread();

                WorkerPool_P* const pool;

                virtual void run();

                QWaitCondition wakeup;

            friend class OsmAnd::Concurrent::WorkerPool_P;
            };

            QAtomicInt _order;
            QAtomicInt _maxThreadCount;

            mutable QMutex _mutex;
            QVector<QRunnable*> _queue;
            QSet<WorkerThread*> _allThreads;
            QQueue<WorkerThread*> _freeThreads;
            QQueue<WorkerThread*> _inactiveThreads;
            volatile bool _isBeingReset;
            mutable QWaitCondition _threadFreed;

            unsigned int activeThreadCountNoLock() const;
            void createNewThread();
            bool tryLaunchNextRunnable();
            void tryLaunchNextRunnables();
            QRunnable* takeNextRunnable();
            bool tooManyThreadsActive() const;
            void dequeueAllNoLock();
            bool waitForDoneNoLock(const int msecs) const;
            void sortQueueNoLock(const SortPredicate predicate);
        protected:
            WorkerPool_P(WorkerPool* const owner, const Order order, const int maxThreadCount);
        public:
            ~WorkerPool_P();

            ImplementationInterface<WorkerPool> owner;

            Order order() const;
            void setOrder(const Order order);

            int maxThreadCount() const;
            void setMaxThreadCount(int maxThreadCount);
            unsigned int activeThreadCount() const;

            bool waitForDone(const int msecs) const;

            void enqueue(QRunnable* const runnable, const SortPredicate predicate);
            void enqueue(const QVector<QRunnable*>& runnables, const SortPredicate predicate);
            bool dequeue(QRunnable* const runnable, const SortPredicate predicate);
            void dequeueAll();

            void sortQueue(const SortPredicate predicate);

            void reset();

        friend class OsmAnd::Concurrent::WorkerPool;
        friend class OsmAnd::Concurrent::WorkerPool_P::WorkerThread;
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_WORKER_POOL_P_H_)
