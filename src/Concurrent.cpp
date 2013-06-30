#include "Concurrent.h"

#include <assert.h>

std::unique_ptr<OsmAnd::Concurrent> OsmAnd::Concurrent::_instance(new OsmAnd::Concurrent());

OsmAnd::Concurrent::Concurrent()
    : localStoragePool(new QThreadPool())
    , networkPool(new QThreadPool())
{
    assert(!_instance);

    localStoragePool->setMaxThreadCount(1);
    networkPool->setMaxThreadCount(1);
}

OsmAnd::Concurrent::~Concurrent()
{
}

const std::unique_ptr<OsmAnd::Concurrent>& OsmAnd::Concurrent::instance()
{
    return _instance;
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
