#include "Concurrent.h"

#include <assert.h>

const std::shared_ptr<OsmAnd::Concurrent::Pools> OsmAnd::Concurrent::Pools::instance(new OsmAnd::Concurrent::Pools());
const std::shared_ptr<OsmAnd::Concurrent::Pools> OsmAnd::Concurrent::pools(OsmAnd::Concurrent::Pools::instance);

OsmAnd::Concurrent::Pools::Pools()
    : localStorage(new QThreadPool())
    , network(new QThreadPool())
{
    localStorage->setMaxThreadCount(1);
    network->setMaxThreadCount(1);
}

OsmAnd::Concurrent::Pools::~Pools()
{
}

OsmAnd::Concurrent::Task::Task( ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/ )
    : _isCancellationRequested(false)
    , preExecute(preExecuteMethod)
    , execute(executeMethod)
    , postExecute(postExecuteMethod)
{
    assert(executeMethod != nullptr);
}

OsmAnd::Concurrent::Task::~Task()
{
}

void OsmAnd::Concurrent::Task::run()
{
    // This local event loop
    QEventLoop localLoop;

    if(preExecute)
        _isCancellationRequested = preExecute(this);
    if(_isCancellationRequested)
        return;

    execute(this, localLoop);

    if(postExecute)
        postExecute(this);
}

OsmAnd::Concurrent::Thread::Thread( ThreadProcedureSignature threadProcedure_ )
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
