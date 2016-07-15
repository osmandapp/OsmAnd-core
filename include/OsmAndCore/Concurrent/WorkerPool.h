#ifndef _OSMAND_CORE_CONCURRENT_WORKER_POOL_H_
#define _OSMAND_CORE_CONCURRENT_WORKER_POOL_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <QRunnable>
#include <QThread>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class WorkerPool_P;
        class OSMAND_CORE_API WorkerPool Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(WorkerPool)

        public:
            enum class Order
            {
                FIFO,
                LIFO,
                Random
            };

            typedef std::function<bool (QRunnable* const l, QRunnable* const r)> SortPredicate;

        private:
            PrivateImplementation<WorkerPool_P> _p;
        protected:
        public:
            WorkerPool(const Order order = Order::FIFO, const int maxThreadCount = QThread::idealThreadCount());
            virtual ~WorkerPool();

            Order order() const;
            void setOrder(const Order order);

            int maxThreadCount() const;
            void setMaxThreadCount(int maxThreadCount);
            unsigned int activeThreadCount() const;

            bool waitForDone(const int msecs = -1) const;

            void enqueue(QRunnable* const runnable, const SortPredicate predicate = nullptr);
            void enqueue(const QVector<QRunnable*>& runnables, const SortPredicate predicate = nullptr);
            bool dequeue(QRunnable* const runnable, const SortPredicate predicate = nullptr);
            void dequeueAll();

            void sortQueue(const SortPredicate predicate);

            void reset();
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_WORKER_POOL_H_)
