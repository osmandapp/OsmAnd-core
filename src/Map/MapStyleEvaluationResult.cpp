#include "MapStyleEvaluationResult.h"
#include "MapStyleEvaluationResult_P.h"

OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult()
    : _d(new MapStyleEvaluationResult_P(this))
{
}

OsmAnd::MapStyleEvaluationResult::~MapStyleEvaluationResult()
{
}

bool OsmAnd::MapStyleEvaluationResult::getBooleanValue(const int valueDefId, bool& value) const
{
    const auto itValue = _d->_podValues.constFind(valueDefId);
    if(itValue == _d->_podValues.cend())
        return false;

    value = (itValue->asInt == 1);
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(const int valueDefId, int& value) const
{
    const auto itValue = _d->_podValues.constFind(valueDefId);
    if(itValue == _d->_podValues.cend())
        return false;

    value = itValue->asInt;
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(const int valueDefId, unsigned int& value) const
{
    const auto itValue = _d->_podValues.constFind(valueDefId);
    if(itValue == _d->_podValues.cend())
        return false;

    value = itValue->asUInt;
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getFloatValue(const int valueDefId, float& value) const
{
    const auto itValue = _d->_podValues.constFind(valueDefId);
    if(itValue == _d->_podValues.cend())
        return false;

    value = itValue->asFloat;
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getStringValue(const int valueDefId, QString& value) const
{
    const auto itValue = _d->_stringValues.constFind(valueDefId);
    if(itValue == _d->_stringValues.cend())
        return false;

    value = *itValue;
    return true;
}

void OsmAnd::MapStyleEvaluationResult::clear()
{
    _d->_podValues.clear();
    _d->_stringValues.clear();
}
