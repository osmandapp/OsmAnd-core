#ifndef _OSMAND_CORE_CONCURRENT_TASK_HOST_H_
#define _OSMAND_CORE_CONCURRENT_TASK_HOST_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QWaitCondition>
#include <QReadWriteLock>
#include <QMutex>
#include <QAtomicInt>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class HostedTask;
        class OSMAND_CORE_API TaskHost : public std::enable_shared_from_this < TaskHost >
        {
            Q_DISABLE_COPY_AND_MOVE(TaskHost)
        public:
            typedef const void* OwnerPtr;

            class OSMAND_CORE_API Bridge
            {
                Q_DISABLE_COPY_AND_MOVE(Bridge)
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

            TaskHost(const OwnerPtr& ownerPtr);
            void onOwnerIsBeingDestructed();
        public:
            ~TaskHost();

            friend class OsmAnd::Concurrent::TaskHost::Bridge;
            friend class OsmAnd::Concurrent::HostedTask;
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_TASK_HOST_H_)
