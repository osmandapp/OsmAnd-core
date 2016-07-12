#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapStyle.h>

namespace OsmAnd
{
    class MapObject;
    class MapStyleEvaluationResult;

    class MapStyleEvaluator_P;
    class OSMAND_CORE_API MapStyleEvaluator Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleEvaluator)
    private:
        PrivateImplementation<MapStyleEvaluator_P> _p;
    protected:
    public:
        MapStyleEvaluator(
            const std::shared_ptr<const IMapStyle>& mapStyle,
            const float ptScaleFactor);
        virtual ~MapStyleEvaluator();

        const std::shared_ptr<const IMapStyle> mapStyle;
        const float ptScaleFactor;

        void setBooleanValue(const IMapStyle::ValueDefinitionId valueDefId, const bool value);
        void setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const int value);
        void setIntegerValue(const IMapStyle::ValueDefinitionId valueDefId, const unsigned int value);
        void setFloatValue(const IMapStyle::ValueDefinitionId valueDefId, const float value);
        void setStringValue(const IMapStyle::ValueDefinitionId valueDefId, const QString& value);

        bool evaluate(
            const std::shared_ptr<const MapObject>& mapObject,
            const MapStyleRulesetType rulesetType,
            MapStyleEvaluationResult* const outResultStorage = nullptr) const;
        bool evaluate(
            const std::shared_ptr<const IMapStyle::IAttribute>& attribute,
            MapStyleEvaluationResult* const outResultStorage = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_H_)
