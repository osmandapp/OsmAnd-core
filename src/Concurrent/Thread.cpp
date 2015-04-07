#include "Thread.h"

OsmAnd::Concurrent::Thread::Thread(ThreadProcedureSignature threadProcedure_)
    : QThread(nullptr)
    , threadProcedure(threadProcedure_)
{
}

OsmAnd::Concurrent::Thread::~Thread()
{
}

void OsmAnd::Concurrent::Thread::run()
{
    assert(threadProcedure);

    // Simply execute procedure
    threadProcedure();
}
