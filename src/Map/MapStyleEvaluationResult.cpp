#include "MapStyleEvaluationResult.h"
#include "MapStyleEvaluationResult_P.h"

#include "QKeyValueIterator.h"

OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult()
    : _p(new MapStyleEvaluationResult_P(this))
{
}

OsmAnd::MapStyleEvaluationResult::~MapStyleEvaluationResult()
{
}

bool OsmAnd::MapStyleEvaluationResult::getBooleanValue(const int valueDefId, bool& value) const
{
    const auto itValue = _p->_values.constFind(valueDefId);
    if (itValue == _p->_values.cend())
        return false;

    value = itValue->toBool();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(const int valueDefId, int& value) const
{
    const auto itValue = _p->_values.constFind(valueDefId);
    if (itValue == _p->_values.cend())
        return false;

    value = itValue->toInt();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(const int valueDefId, unsigned int& value) const
{
    const auto itValue = _p->_values.constFind(valueDefId);
    if (itValue == _p->_values.cend())
        return false;

    value = itValue->toUInt();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getFloatValue(const int valueDefId, float& value) const
{
    const auto itValue = _p->_values.constFind(valueDefId);
    if (itValue == _p->_values.cend())
        return false;

    value = itValue->toFloat();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getStringValue(const int valueDefId, QString& value) const
{
    const auto itValue = _p->_values.constFind(valueDefId);
    if (itValue == _p->_values.cend())
        return false;

    value = itValue->toString();
    return true;
}

void OsmAnd::MapStyleEvaluationResult::clear()
{
    _p->_values.clear();
}

void OsmAnd::MapStyleEvaluationResult::pack(PackedResult& packedResult)
{
    packedResult.reserve(_p->_values.size());
    for(const auto& entry : rangeOf(constOf(_p->_values)))
        packedResult.push_back(qMove(PackedResultEntry(entry.key(), entry.value())));
}
