#include "MapStyleEvaluationResult.h"

#include "QtCommon.h"

#include "QKeyValueIterator.h"

OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult()
{
}

OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult(const MapStyleEvaluationResult& that)
    : values(detachedOf(that.values))
{
}

#ifdef Q_COMPILER_RVALUE_REFS
OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult(MapStyleEvaluationResult&& that)
    : values(qMove(that.values))
{
}
#endif // Q_COMPILER_RVALUE_REFS

OsmAnd::MapStyleEvaluationResult::~MapStyleEvaluationResult()
{
}

OsmAnd::MapStyleEvaluationResult& OsmAnd::MapStyleEvaluationResult::operator=(const MapStyleEvaluationResult& that)
{
    if (this != &that)
        values = detachedOf(that.values);

    return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
OsmAnd::MapStyleEvaluationResult& OsmAnd::MapStyleEvaluationResult::operator=(MapStyleEvaluationResult&& that)
{
    if (this != &that)
        values = qMove(that.values);

    return *this;
}
#endif // Q_COMPILER_RVALUE_REFS

bool OsmAnd::MapStyleEvaluationResult::getBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, bool& value) const
{
    const auto itValue = values.constFind(valueDefId);
    if (itValue == values.cend())
        return false;

    value = itValue->toBool();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, int& value) const
{
    const auto itValue = values.constFind(valueDefId);
    if (itValue == values.cend())
        return false;

    value = itValue->toInt();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, unsigned int& value) const
{
    const auto itValue = values.constFind(valueDefId);
    if (itValue == values.cend())
        return false;

    value = itValue->toUInt();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, float& value) const
{
    const auto itValue = values.constFind(valueDefId);
    if (itValue == values.cend())
        return false;

    value = itValue->toFloat();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, QString& value) const
{
    const auto itValue = values.constFind(valueDefId);
    if (itValue == values.cend())
        return false;

    value = itValue->toString();
    return true;
}

void OsmAnd::MapStyleEvaluationResult::clear()
{
    values.clear();
}

bool OsmAnd::MapStyleEvaluationResult::isEmpty() const
{
    return values.isEmpty();
}

void OsmAnd::MapStyleEvaluationResult::pack(PackedResult& packedResult)
{
    packedResult.reserve(values.size());
    for(const auto& entry : rangeOf(constOf(values)))
        packedResult.push_back(qMove(PackedResultEntry(entry.key(), entry.value())));
}
