#ifndef _OSMAND_CORE_CONCURRENT_DISPATCHER_H_
#define _OSMAND_CORE_CONCURRENT_DISPATCHER_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>


#include "OsmAndCoreAPI.h"

namespace OsmAnd
{
    namespace Concurrent
    {
        class OSMAND_CORE_API Dispatcher
        {
        public:
            Dispatcher() = default;
            Dispatcher(const Dispatcher&) = delete;
            Dispatcher(Dispatcher&&) = delete;
            Dispatcher& operator=(const Dispatcher&) = delete;
            Dispatcher& operator=(Dispatcher&&) = delete;

            using Delegate = std::function<void()>;

        protected:
            mutable std::mutex _queueMutex;
            std::queue<Delegate> _queue;
            std::condition_variable _queueCondition;

            std::mutex _runnerMutex;
            bool _isRunningStandalone;
            bool _shutdownRequested;
            std::condition_variable _runnerCondition;

        public:

            int queueSize() const;

            void runAll();
            bool runOne();
            void run();

            void invoke(Delegate method);
            void invokeAsync(Delegate method);

            void shutdown();
            void shutdownAsync();
        };
    } // namespace Concurrent
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_CONCURRENT_DISPATCHER_H_)
