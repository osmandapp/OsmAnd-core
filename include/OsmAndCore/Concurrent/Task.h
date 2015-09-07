#ifndef _OSMAND_CORE_CONCURRENT_TASK_H_
#define _OSMAND_CORE_CONCURRENT_TASK_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QRunnable>
#include <QAtomicInt>
#include <QReadWriteLock>
#include <OsmAndCore/restore_internal_warnings.h>

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

            class OSMAND_CORE_API Cancellator Q_DECL_FINAL
            {
                Q_DISABLE_COPY_AND_MOVE(Cancellator);
            private:
                mutable QReadWriteLock _taskRefLock;
                Task* _taskRef;
            protected:
                Cancellator(Task* const taskRef);

                void unlink();
            public:
                virtual ~Cancellator();

                bool isLinked() const;

                void requestCancellation(bool* const pOutSuccess = nullptr);
                bool isCancellationRequested(bool* const pOutSuccess = nullptr) const;

                friend class OsmAnd::Concurrent::Task;
            };

        private:
            bool _cancellationRequestedByTask;
            QAtomicInt _cancellationRequestedByExternal;

            std::shared_ptr<Cancellator> _cancellator;
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

            void requestCancellation();
            bool isCancellationRequested() const;

            std::weak_ptr<Cancellator> obtainCancellator();
            std::weak_ptr<const Cancellator> obtainCancellator() const;

            virtual void run();
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_TASK_H_)
