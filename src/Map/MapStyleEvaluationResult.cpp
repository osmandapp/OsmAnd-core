#include "MapStyleEvaluationResult.h"

#include "QtCommon.h"

OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult(const int size /*= 0*/)
    : _storage(size)
{
}

OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult(const MapStyleEvaluationResult& that)
    : _storage(detachedOf(that._storage))
{
}

#ifdef Q_COMPILER_RVALUE_REFS
OsmAnd::MapStyleEvaluationResult::MapStyleEvaluationResult(MapStyleEvaluationResult&& that)
    : _storage(qMove(that._storage))
{
}
#endif // Q_COMPILER_RVALUE_REFS

OsmAnd::MapStyleEvaluationResult::~MapStyleEvaluationResult()
{
}

OsmAnd::MapStyleEvaluationResult& OsmAnd::MapStyleEvaluationResult::operator=(const MapStyleEvaluationResult& that)
{
    if (this != &that)
        _storage = detachedOf(that._storage);

    return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
OsmAnd::MapStyleEvaluationResult& OsmAnd::MapStyleEvaluationResult::operator=(MapStyleEvaluationResult&& that)
{
    if (this != &that)
        _storage = qMove(that._storage);

    return *this;
}
#endif // Q_COMPILER_RVALUE_REFS

bool OsmAnd::MapStyleEvaluationResult::contains(const IMapStyle::ValueDefinitionId valueDefId) const
{
    if (valueDefId >= _storage.size())
        return false;

    return _storage[valueDefId].isValid();
}

void OsmAnd::MapStyleEvaluationResult::setValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const QVariant& value)
{
    if (valueDefId >= _storage.size())
        _storage.resize(valueDefId + 1);

    _storage[valueDefId] = value;
}

void OsmAnd::MapStyleEvaluationResult::setBooleanValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const bool& value)
{
    setValue(valueDefId, QVariant(value));
}

void OsmAnd::MapStyleEvaluationResult::setIntegerValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const int& value)
{
    setValue(valueDefId, QVariant(value));
}

void OsmAnd::MapStyleEvaluationResult::setIntegerValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const unsigned int& value)
{
    setValue(valueDefId, QVariant(value));
}

void OsmAnd::MapStyleEvaluationResult::setFloatValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const float& value)
{
    setValue(valueDefId, QVariant(value));
}

void OsmAnd::MapStyleEvaluationResult::setStringValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const QString& value)
{
    setValue(valueDefId, QVariant(value));
}

bool OsmAnd::MapStyleEvaluationResult::getValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    QVariant& outValue) const
{
    if (valueDefId >= _storage.size())
        return false;

    const auto& value = _storage[valueDefId];
    if (!value.isValid())
        return false;

    outValue = value;
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getBooleanValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    bool& outValue) const
{
    QVariant value;
    if (!getValue(valueDefId, value))
        return false;

    outValue = value.toBool();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    int& outValue) const
{
    QVariant value;
    if (!getValue(valueDefId, value))
        return false;

    outValue = value.toInt();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getIntegerValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    unsigned int& outValue) const
{
    QVariant value;
    if (!getValue(valueDefId, value))
        return false;

    outValue = value.toUInt();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getFloatValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    float& outValue) const
{
    QVariant value;
    if (!getValue(valueDefId, value))
        return false;

    outValue = value.toFloat();
    return true;
}

bool OsmAnd::MapStyleEvaluationResult::getStringValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    QString& outValue) const
{
    QVariant value;
    if (!getValue(valueDefId, value))
        return false;

    outValue = value.toString();
    return true;
}

QHash<OsmAnd::IMapStyle::ValueDefinitionId, QVariant> OsmAnd::MapStyleEvaluationResult::getValues() const
{
    QHash<IMapStyle::ValueDefinitionId, QVariant> result;

    const auto size = _storage.size();
    auto pValue = _storage.constData();
    int valuesCount = 0;
    for (int index = 0; index < size; index++)
    {
        const auto& value = *(pValue++);
        if (!value.isValid())
            continue;

        result.insert(static_cast<IMapStyle::ValueDefinitionId>(index), value);
    }

    return result;
}

void OsmAnd::MapStyleEvaluationResult::reset()
{
    _storage.clear();
}

void OsmAnd::MapStyleEvaluationResult::clear()
{
    const auto size = _storage.size();
    auto pValue = _storage.data();
    for (int index = 0; index < size; index++)
        (pValue++)->clear();
}

bool OsmAnd::MapStyleEvaluationResult::isEmpty() const
{
    const auto size = _storage.size();
    auto pValue = _storage.constData();
    int valuesCount = 0;
    for (int index = 0; index < size; index++)
    {
        if ((pValue++)->isValid())
            return false;
    }

    return true;
}

void OsmAnd::MapStyleEvaluationResult::pack(PackedResult& packedResult) const
{
    const auto size = _storage.size();

    auto pValue = _storage.constData();
    int valuesCount = 0;
    for (int index = 0; index < size; index++)
    {
        if ((pValue++)->isValid())
            valuesCount++;
    }

    pValue = _storage.constData();
    packedResult.reserve(valuesCount);
    for (int index = 0; index < size; index++)
        packedResult.push_back(qMove(PackedResultEntry(index, *(pValue++))));
}
