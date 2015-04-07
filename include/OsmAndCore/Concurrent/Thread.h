#ifndef _OSMAND_CORE_CONCURRENT_THREAD_H_
#define _OSMAND_CORE_CONCURRENT_THREAD_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QThread>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace Concurrent
    {
        class OSMAND_CORE_API Thread : public QThread
        {
        public:
            typedef std::function<void()> ThreadProcedureSignature;
        private:
        protected:
            virtual void run();
        public:
            Thread(ThreadProcedureSignature threadProcedure);
            virtual ~Thread();

            const ThreadProcedureSignature threadProcedure;
        };
    }
}

#endif // !defined(_OSMAND_CORE_CONCURRENT_THREAD_H_)
