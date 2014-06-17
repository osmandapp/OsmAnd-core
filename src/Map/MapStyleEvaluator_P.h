#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMap>

#include "OsmAndCore.h"
#include "MapStyle.h"

namespace OsmAnd
{
    class MapStyleRule;
    class MapStyleEvaluationResult;
    class MapStyleBuiltinValueDefinitions;
    namespace Model
    {
        class BinaryMapObject;
    }
    
    class MapStyleEvaluator;
    class MapStyleEvaluator_P
    {
    public:
        union InputValue
        {
            inline InputValue()
            {
                asUInt = 0;
            }

            float asFloat;
            int32_t asInt;
            uint32_t asUInt;
        };
    private:
    protected:
        MapStyleEvaluator_P(MapStyleEvaluator* owner);

        ImplementationInterface<MapStyleEvaluator> owner;

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        QMap<int, InputValue> _inputValues;

        bool evaluate(
            const Model::BinaryMapObject* const mapObject,
            const std::shared_ptr<const MapStyleRule>& singleRule,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren);
        bool evaluate(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
            const QMap< uint64_t, std::shared_ptr<MapStyleRule> >& rules,
            const uint32_t tagKey, const uint32_t valueKey,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren);
    public:
        ~MapStyleEvaluator_P();

        bool evaluate(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject, const MapStyleRulesetType ruleset,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren);
        bool evaluate(
            const std::shared_ptr<const MapStyleRule>& singleRule,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren);

    friend class OsmAnd::MapStyleEvaluator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_)
