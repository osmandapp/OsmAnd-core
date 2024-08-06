#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "OnDemand.h"
#include "ArrayMap.h"
#include "PrivateImplementation.h"
#include "MapStyleConstantValue.h"
#include "IMapStyle.h"

namespace OsmAnd
{
    class MapStyleEvaluationResult;
    class MapStyleBuiltinValueDefinitions;
    class MapObject;

    class MapStyleEvaluator;
    class MapStyleEvaluator_P Q_DECL_FINAL
    {
    public:
        union InputValue
        {
            inline InputValue()
                : asUInt(0u)
            {
            }

            float asFloat;
            int32_t asInt;
            uint32_t asUInt;
        };

    private:
        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        typedef ArrayMap<InputValue> InputValues;
        std::shared_ptr<InputValues> _inputValues;
        std::shared_ptr<InputValues> _inputValuesShadow;

        typedef ArrayMap<IMapStyle::Value> IntermediateEvaluationResult;
        std::shared_ptr<IntermediateEvaluationResult> _intermediateEvaluationResult;
        std::shared_ptr<IntermediateEvaluationResult> _constantIntermediateEvaluationResult;

        void prepare();

        ArrayMap<IMapStyle::Value>* allocateIntermediateEvaluationResult();
        
        MapStyleConstantValue evaluateConstantValue(
            const MapObject* const mapObject,
            const MapStyleValueDataType dataType,
            const IMapStyle::Value& resolvedValue,
            const std::shared_ptr<const InputValues>& inputValues,
            IntermediateEvaluationResult* const outResultStorage,
            OnDemand<IntermediateEvaluationResult>& intermediateEvaluationResult) const;

        bool evaluate(
            const MapObject* const mapObject,
            const std::shared_ptr<const IMapStyle::IRuleNode>& ruleNode,
            const std::shared_ptr<const InputValues>& inputValues,
            bool& outDisabled,
            IntermediateEvaluationResult* const outResultStorage,
            OnDemand<IntermediateEvaluationResult>& constantEvaluationResult) const;

        bool evaluate(
            const std::shared_ptr<const MapObject>& mapObject,
            const QHash< TagValueId, std::shared_ptr<const IMapStyle::IRule> >& ruleset,
            const IMapStyle::StringId tagStringId,
            const IMapStyle::StringId valueStringId,
            MapStyleEvaluationResult* const outResultStorage,
            OnDemand<IntermediateEvaluationResult>& constantEvaluationResult) const;

        void fillResultFromRuleNode(
            const std::shared_ptr<const IMapStyle::IRuleNode>& ruleNode,
            IntermediateEvaluationResult& outResultStorage,
            const bool allowOverride) const;

        void postprocessEvaluationResult(
            const MapObject* const mapObject,
            const std::shared_ptr<const InputValues>& inputValues,
            IntermediateEvaluationResult& intermediateResult,
            MapStyleEvaluationResult& outResultStorage,
            OnDemand<IntermediateEvaluationResult>& constantEvaluationResult) const;
    protected:
        MapStyleEvaluator_P(MapStyleEvaluator* owner);
    public:
        ~MapStyleEvaluator_P();

        ImplementationInterface<MapStyleEvaluator> owner;

        const OnDemand<IntermediateEvaluationResult>::Allocator intermediateEvaluationResultAllocator;

        void setBooleanValue(const IMapStyle::ValueDefinitionId valueDefId, const bool value);
        void setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const int value);
        void setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const unsigned int value);
        void setFloatValue(const IMapStyle::ValueDefinitionId valueDefId, const float value);
        void setStringValue(const IMapStyle::ValueDefinitionId valueDefId, const QString& value);

        bool evaluate(
            const std::shared_ptr<const MapObject>& mapObject,
            const MapStyleRulesetType rulesetType,
            MapStyleEvaluationResult* const outResultStorage) const;
        bool evaluate(
            const std::shared_ptr<const IMapStyle::IAttribute>& attribute,
            MapStyleEvaluationResult* const outResultStorage) const;

    friend class OsmAnd::MapStyleEvaluator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_)
