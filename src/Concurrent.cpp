#include "Concurrent.h"

#include <assert.h>

std::unique_ptr<OsmAnd::Concurrent> OsmAnd::Concurrent::_instance(new OsmAnd::Concurrent());

OsmAnd::Concurrent::Concurrent()
    : localStoragePool(new QThreadPool())
    , networkPool(new QThreadPool())
{
    assert(!_instance);
}

OsmAnd::Concurrent::~Concurrent()
{
}

OsmAnd::Concurrent::Task::Task( ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod /*= nullptr*/, PostExecuteSignature postExecuteMethod /*= nullptr*/ )
{

}

OsmAnd::Concurrent::Task::~Task()
{

}
