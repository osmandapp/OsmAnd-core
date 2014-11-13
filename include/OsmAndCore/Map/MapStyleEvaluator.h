#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/ResolvedMapStyle.h>

namespace OsmAnd
{
    class MapObject;
    struct MapStyleEvaluationResult;

    class MapStyleEvaluator_P;
    class OSMAND_CORE_API MapStyleEvaluator Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleEvaluator);
    private:
        PrivateImplementation<MapStyleEvaluator_P> _p;
    protected:
    public:
        MapStyleEvaluator(const std::shared_ptr<const ResolvedMapStyle>& resolvedStyle, const float displayDensityFactor);
        virtual ~MapStyleEvaluator();

        const std::shared_ptr<const ResolvedMapStyle> resolvedStyle;
        const float displayDensityFactor;

        void setBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const bool value);
        void setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const int value);
        void setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const unsigned int value);
        void setFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const float value);
        void setStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const QString& value);

        bool evaluate(
            const std::shared_ptr<const MapObject>& mapObject,
            const MapStyleRulesetType rulesetType,
            MapStyleEvaluationResult* const outResultStorage = nullptr) const;
        bool evaluate(
            const std::shared_ptr<const ResolvedMapStyle::Attribute>& attribute,
            MapStyleEvaluationResult* const outResultStorage = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_H_)
