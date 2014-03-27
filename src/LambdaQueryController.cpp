#include "LambdaQueryController.h"

OsmAnd::LambdaQueryController::LambdaQueryController(Signature function)
    : _function(function)
{
}

OsmAnd::LambdaQueryController::~LambdaQueryController()
{
}

bool OsmAnd::LambdaQueryController::isAborted() const
{
    return _function();
}
