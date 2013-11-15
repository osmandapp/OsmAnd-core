#include "MapStyleEvaluatorState.h"
#include "MapStyleEvaluatorState_P.h"

OsmAnd::MapStyleEvaluatorState::MapStyleEvaluatorState()
    : _d(new MapStyleEvaluatorState_P(this))
{
}

OsmAnd::MapStyleEvaluatorState::~MapStyleEvaluatorState()
{
}

void OsmAnd::MapStyleEvaluatorState::clear()
{
    _d->_values.clear();
}
