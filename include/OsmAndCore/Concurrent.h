/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
        class OSMAND_CORE_API Pools Q_DECL_FINAL
        {
            Q_DISABLE_COPY(Pools);
        private:
        protected:
            Pools();
        public:
            virtual ~Pools();

            static const std::shared_ptr<Pools> instance;

            const std::unique_ptr<QThreadPool> localStorage;
            const std::unique_ptr<QThreadPool> network;
        };
        const extern OSMAND_CORE_API std::shared_ptr<Pools> pools;

        class OSMAND_CORE_API Task : public QRunnable
        {
            Q_DISABLE_COPY(Task);
        public:
            typedef std::function<void (Task*, bool& requestCancellation)> PreExecuteSignature;
            typedef std::function<void (Task*, QEventLoop& eventLoop)> ExecuteSignature;
            typedef std::function<void (Task*, bool wasCancelled)> PostExecuteSignature;
        private:
            volatile bool _cancellationRequestedByTask;
            volatile bool _cancellationRequestedByExternal;
            mutable QMutex _cancellationMutex;
        protected:
        public:
            Task(ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod = nullptr, PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~Task();

            const PreExecuteSignature preExecute;
            const ExecuteSignature execute;
            const PostExecuteSignature postExecute;

            bool requestCancellation();
            bool isCancellationRequested() const;

            virtual void run();
        };

        class HostedTask;
        class OSMAND_CORE_API TaskHost : public std::enable_shared_from_this<TaskHost>
        {
            Q_DISABLE_COPY(TaskHost)
        public:
            typedef void* OwnerPtr;

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
            Q_OBJECT
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
    } // namespace Concurrent
} // namespace OsmAnd

#endif // _OSMAND_CORE_CONCURRENT_H_
