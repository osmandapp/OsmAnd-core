#include "FunctorQueryController.h"

OsmAnd::FunctorQueryController::FunctorQueryController(const Callback callback_)
    : _callback(callback_)
{
}

OsmAnd::FunctorQueryController::~FunctorQueryController()
{
}

bool OsmAnd::FunctorQueryController::isAborted() const
{
    return _callback(this);
}
