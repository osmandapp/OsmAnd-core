#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "ResolvedMapStyle.h"

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
                : asUInt(0u)
            {
            }

            float asFloat;
            int32_t asInt;
            uint32_t asUInt;
        };

    private:
        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        typedef QHash<ResolvedMapStyle::ValueDefinitionId, InputValue> InputValuesDictionary;
        InputValuesDictionary _inputValues;

        bool evaluate(
            const Model::BinaryMapObject* const mapObject,
            const std::shared_ptr<const ResolvedMapStyle::RuleNode>& ruleNode,
            const InputValuesDictionary& inputValues,
            MapStyleEvaluationResult* const outResultStorage) const;

        bool evaluate(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
            const QHash< ResolvedMapStyle::TagValueId, std::shared_ptr<const ResolvedMapStyle::Rule> >& ruleset,
            const ResolvedMapStyle::StringId tagStringId,
            const ResolvedMapStyle::StringId valueStringId,
            MapStyleEvaluationResult* const outResultStorage) const;

        void fillResultFromRuleNode(
            const std::shared_ptr<const ResolvedMapStyle::RuleNode>& ruleNode,
            MapStyleEvaluationResult& outResultStorage,
            const bool allowOverride) const;
    protected:
        MapStyleEvaluator_P(MapStyleEvaluator* owner);
    public:
        ~MapStyleEvaluator_P();

        ImplementationInterface<MapStyleEvaluator> owner;

        void setBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const bool value);
        void setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const int value);
        void setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const unsigned int value);
        void setFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const float value);
        void setStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const QString& value);

        bool evaluate(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
            const MapStyleRulesetType rulesetType,
            MapStyleEvaluationResult* const outResultStorage) const;
        bool evaluate(
            const std::shared_ptr<const ResolvedMapStyle::Attribute>& attribute,
            MapStyleEvaluationResult* const outResultStorage) const;

    friend class OsmAnd::MapStyleEvaluator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_P_H_)
