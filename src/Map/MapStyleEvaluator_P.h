#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "MapStyle.h"

namespace OsmAnd
{
    class MapStyleNode;
    struct MapStyleEvaluationResult;
    class MapStyleBuiltinValueDefinitions;
    namespace Model
    {
        class BinaryMapObject;
    }

    class MapStyleEvaluator;
    class MapStyleEvaluator_P Q_DECL_FINAL
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
        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        typedef QHash<int, InputValue> InputValuesDictionary;
        InputValuesDictionary _inputValues;

        bool evaluate(
            const Model::BinaryMapObject* const mapObject,
            const std::shared_ptr<const MapStyleRule>& rule,
            const InputValuesDictionary& inputValues,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren) const;

        bool evaluate(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
            const QMap< uint64_t, std::shared_ptr<const MapStyleRule> >& rules,
            const uint32_t tagKey,
            const uint32_t valueKey,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren,
            std::shared_ptr<const MapStyleRule>* outMatchedRule) const;
    protected:
        MapStyleEvaluator_P(MapStyleEvaluator* owner);
    public:
        ~MapStyleEvaluator_P();

        ImplementationInterface<MapStyleEvaluator> owner;

        void setBooleanValue(const int valueDefId, const bool value);
        void setIntegerValue(const int valueDefId, const int value);
        void setIntegerValue(const int valueDefId, const unsigned int value);
        void setFloatValue(const int valueDefId, const float value);
        void setStringValue(const int valueDefId, const QString& value);

        bool evaluate(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
            const MapStyleRulesetType ruleset,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren,
            std::shared_ptr<const MapStyleRule>* outMatchedRule) const;
        bool evaluate(
            const std::shared_ptr<const MapStyleRule>& singleRule,
            MapStyleEvaluationResult* const outResultStorage,
            bool evaluateChildren) const;

    friend class OsmAnd::MapStyleEvaluator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_)
