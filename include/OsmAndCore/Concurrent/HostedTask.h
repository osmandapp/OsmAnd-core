#ifndef _OSMAND_CORE_CONCURRENT_HOSTED_TASK_H_
#define _OSMAND_CORE_CONCURRENT_HOSTED_TASK_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Concurrent/Task.h>
#include <OsmAndCore/Concurrent/TaskHost.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class OSMAND_CORE_API HostedTask : public Task
        {
            Q_DISABLE_COPY_AND_MOVE(HostedTask)
        private:
        protected:
            std::shared_ptr<TaskHost> _host;

            TaskHost::OwnerPtr _lockedOwner;
        public:
            HostedTask(
                const TaskHost::Bridge& bridge,
                const ExecuteSignature executeFunctor,
                const PreExecuteSignature preExecuteFunctor = nullptr,
                const PostExecuteSignature postExecuteFunctor = nullptr);
            virtual ~HostedTask();

            virtual void run();

            TaskHost::OwnerPtr& lockedOwner;
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_HOSTED_TASK_H_)
