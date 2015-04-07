#ifndef _OSMAND_CORE_CONCURRENT_TASK_H_
#define _OSMAND_CORE_CONCURRENT_TASK_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QRunnable>
#include <QAtomicInt>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class OSMAND_CORE_API Task : public QRunnable
        {
            Q_DISABLE_COPY_AND_MOVE(Task);
        public:
            typedef std::function<void(Task* const task, bool& requestCancellation)> PreExecuteSignature;
            typedef std::function<void(Task* const task)> ExecuteSignature;
            typedef std::function<void(Task* const task, const bool wasCancelled)> PostExecuteSignature;
        private:
            bool _cancellationRequestedByTask;
            QAtomicInt _cancellationRequestedByExternal;
        protected:
        public:
            Task(
                const ExecuteSignature executeFunctor,
                const PreExecuteSignature preExecuteFunctor = nullptr,
                const PostExecuteSignature postExecuteFunctor = nullptr);
            virtual ~Task();

            const PreExecuteSignature preExecuteFunctor;
            const ExecuteSignature executeFunctor;
            const PostExecuteSignature postExecuteFunctor;

            virtual void requestCancellation();
            bool isCancellationRequested() const;

            virtual void run();
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_TASK_H_)
