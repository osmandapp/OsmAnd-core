#include "SimpleQueryController.h"

OsmAnd::SimpleQueryController::SimpleQueryController()
    : _isAborted(0)
{
}

OsmAnd::SimpleQueryController::~SimpleQueryController()
{
}

void OsmAnd::SimpleQueryController::abort()
{
    _isAborted.testAndSetOrdered(0, 1);
}

bool OsmAnd::SimpleQueryController::isAborted() const
{
    return (_isAborted.loadAcquire() != 0);
}
