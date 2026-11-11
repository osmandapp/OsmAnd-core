#include "MapStyleEvaluator.h"
#include "MapStyleEvaluator_P.h"
#include "MapStyleEvaluationResult.h"

#include <cassert>

#include "Logging.h"

#include <string>
#include <QHash>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace OsmAnd
{
    bool& zbStyleTouched();
}

namespace
{
    bool zbFlagH()
    {
        static const bool enabled = []() -> bool
        {
            char buffer[96] = { 0 };
#if defined(__ANDROID__)
            __system_property_get("debug.osmand.zb", buffer);
#endif
            return std::string(buffer).find('H') != std::string::npos;
        }();
        return enabled;
    }

    struct ZbStyleValue
    {
        bool ok = false;
        bool dependent = false;
        OsmAnd::MapStyleEvaluationResult result;
    };
    struct ZbStyleCache
    {
        QHash<quint64, ZbStyleValue> entries;
        quint64 hits = 0, misses = 0, dependent = 0;
    };
    ZbStyleCache& zbStyleCache()
    {
        static thread_local ZbStyleCache cache;
        return cache;
    }
    inline void zbMixH(quint64& h, const quint64 v)
    {
        h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
    }
}


OsmAnd::MapStyleEvaluator::MapStyleEvaluator(
    const std::shared_ptr<const IMapStyle>& mapStyle_,
    const float ptScaleFactor_)
    : _p(new MapStyleEvaluator_P(this))
    , mapStyle(mapStyle_)
    , ptScaleFactor(ptScaleFactor_)
{
    if (mapStyle_)
        _p->prepare();
}

OsmAnd::MapStyleEvaluator::~MapStyleEvaluator()
{
}

void OsmAnd::MapStyleEvaluator::setBooleanValue(const IMapStyle::ValueDefinitionId valueDefId, const bool value)
{
    _p->setBooleanValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const unsigned int value)
{
    _p->setIntegerValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setFloatValue(const IMapStyle::ValueDefinitionId valueDefId, const float value)
{
    _p->setFloatValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setStringValue(const IMapStyle::ValueDefinitionId valueDefId, const QString& value)
{
    _p->setStringValue(valueDefId, value);
}

void OsmAnd::MapStyleEvaluator::setStringIdValue(
    const IMapStyle::ValueDefinitionId valueDefId,
    const IMapStyle::StringId value)
{
    _p->setStringIdValue(valueDefId, value);
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const MapObject>& mapObject,
    const MapStyleRulesetType rulesetType,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/) const
{
    if (!zbFlagH() || (outResultStorage && !outResultStorage->isEmpty()))
        return _p->evaluate(mapObject, rulesetType, outResultStorage);

    // key: style, ruleset, whether a result is wanted, every input value that is set
    quint64 key = reinterpret_cast<quintptr>(mapStyle.get());
    zbMixH(key, static_cast<quint64>(rulesetType) * 2 + (outResultStorage ? 1 : 0));
    const auto& inputs = *_p->_inputValues;
    for (const auto inputId : inputs.getSetKeysRef())
    {
        const auto value = inputs.getRef(inputId);
        if (!value)
            continue;
        zbMixH(key, (static_cast<quint64>(inputId) << 32) | value->asUInt);
    }

    auto& cache = zbStyleCache();
    const auto it = cache.entries.constFind(key);
    if (it != cache.entries.cend() && !it->dependent)
    {
        cache.hits++;
        if (outResultStorage && it->ok)
            *outResultStorage = it->result;
        return it->ok;
    }

    const bool known = it != cache.entries.cend();
    zbStyleTouched() = false;
    const bool ok = _p->evaluate(mapObject, rulesetType, outResultStorage);
    if (known)
    {
        cache.dependent++;
        return ok;
    }
    cache.misses++;
    if (cache.entries.size() > 100000)
        cache.entries.clear();
    ZbStyleValue v;
    v.ok = ok;
    v.dependent = zbStyleTouched();
    if (ok && outResultStorage && !v.dependent)
        v.result = *outResultStorage;
    cache.entries.insert(key, v);
    if (((cache.hits + cache.misses + cache.dependent) & 0x3FFF) == 0)
        LogPrintf(LogSeverityLevel::Info, ">>>> 0 STYLECACHE 0.0: hits=%llu misses=%llu dependent=%llu entries=%d",
            cache.hits, cache.misses, cache.dependent, cache.entries.size());
    return ok;
}

bool OsmAnd::MapStyleEvaluator::evaluate(
    const std::shared_ptr<const IMapStyle::IAttribute>& attribute,
    MapStyleEvaluationResult* const outResultStorage /*= nullptr*/) const
{
    return _p->evaluate(attribute, outResultStorage);
}
