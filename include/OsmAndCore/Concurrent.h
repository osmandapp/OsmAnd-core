#ifndef _OSMAND_CORE_CONCURRENT_H_
#define _OSMAND_CORE_CONCURRENT_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QThreadPool>
#include <QEventLoop>
#include <QRunnable>
#include <QWaitCondition>
#include <QReadWriteLock>
#include <QMutex>
#include <QAtomicInt>
#include <QQueue>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class OSMAND_CORE_API Task : public QRunnable
        {
            Q_DISABLE_COPY(Task);
        public:
            typedef std::function<void (Task* task, bool& requestCancellation)> PreExecuteSignature;
            typedef std::function<void (Task* task)> ExecuteSignature;
            typedef std::function<void (Task* task, bool wasCancelled)> PostExecuteSignature;
        private:
            bool _cancellationRequestedByTask;
            QAtomicInt _cancellationRequestedByExternal;
        protected:
        public:
            Task(ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod = nullptr, PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~Task();

            const PreExecuteSignature preExecute;
            const ExecuteSignature execute;
            const PostExecuteSignature postExecute;

            void requestCancellation();
            bool isCancellationRequested() const;

            virtual void run();
        };

        class HostedTask;
        class OSMAND_CORE_API TaskHost : public std::enable_shared_from_this<TaskHost>
        {
            Q_DISABLE_COPY(TaskHost)
        public:
            typedef const void* OwnerPtr;

            class OSMAND_CORE_API Bridge
            {
                Q_DISABLE_COPY(Bridge)
            private:
            protected:
                std::shared_ptr<TaskHost> _host;
            public:
                Bridge(const OwnerPtr& owner);
                ~Bridge();

                void onOwnerIsBeingDestructed() const;

            friend class OsmAnd::Concurrent::HostedTask;
            };
        private:
        protected:
            const OwnerPtr _ownerPtr;
            QList< HostedTask* > _hostedTasks;
            mutable QReadWriteLock _hostedTasksLock;
            QWaitCondition _unlockedCondition;
            volatile bool _ownerIsBeingDestructed;

            TaskHost(const OwnerPtr& owner);
            void onOwnerIsBeingDestructed();
        public:
            ~TaskHost();

        friend class OsmAnd::Concurrent::TaskHost::Bridge;
        friend class OsmAnd::Concurrent::HostedTask;
        };

        class OSMAND_CORE_API HostedTask : public Task
        {
            Q_DISABLE_COPY(HostedTask);
        private:
        protected:
            std::shared_ptr<TaskHost> _host;

            TaskHost::OwnerPtr _lockedOwner;
        public:
            HostedTask(const TaskHost::Bridge& bridge, ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod = nullptr, PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~HostedTask();

            virtual void run();

            const TaskHost::OwnerPtr& lockedOwner;
        };

        class OSMAND_CORE_API Thread : public QThread
        {
        public:
            typedef std::function<void ()> ThreadProcedureSignature;
        private:
        protected:
            virtual void run();
        public:
            Thread(ThreadProcedureSignature threadProcedure);
            virtual ~Thread();

            const ThreadProcedureSignature threadProcedure;
        };

        class OSMAND_CORE_API Dispatcher
        {
            Q_DISABLE_COPY(Dispatcher);
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

#endif // !defined(_OSMAND_CORE_CONCURRENT_H_)
