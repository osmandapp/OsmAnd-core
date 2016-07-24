#ifndef _OSMAND_CORE_CONCURRENT_DISPATCHER_H_
#define _OSMAND_CORE_CONCURRENT_DISPATCHER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QWaitCondition>
#include <QMutex>
#include <QAtomicInt>
#include <QQueue>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class OSMAND_CORE_API Dispatcher
        {
            Q_DISABLE_COPY_AND_MOVE(Dispatcher)
        public:
            typedef std::function<void()> Delegate;
        private:
        protected:
            mutable QMutex _queueMutex;
            QQueue< Delegate > _queue;

            volatile bool _isRunningStandalone;
            volatile bool _shutdownRequested;
            QMutex _performedShutdownConditionMutex;
            QWaitCondition _performedShutdown;
        public:
            Dispatcher();
            virtual ~Dispatcher();

            int queueSize() const;

            void runAll();
            void runOne();
            void run();

            void invoke(const Delegate method);
            void invokeAsync(const Delegate method);

            void shutdown();
            void shutdownAsync();
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_DISPATCHER_H_)
