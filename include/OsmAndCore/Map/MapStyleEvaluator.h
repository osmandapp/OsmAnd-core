#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATOR_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyle.h>

namespace OsmAnd
{
    namespace Model
    {
        class MapObject;
    }
    class MapStyleRule;
    class MapStyleValueDefinition;
    struct MapStyleValue;
    class MapStyleEvaluationResult;

    class MapStyleEvaluator_P;
    class OSMAND_CORE_API MapStyleEvaluator
    {
        Q_DISABLE_COPY(MapStyleEvaluator);
    private:
        const std::unique_ptr<MapStyleEvaluator_P> _d;
    protected:
    public:
        MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style, const float displayDensityFactor);
        virtual ~MapStyleEvaluator();

        const std::shared_ptr<const MapStyle> style;
        const float displayDensityFactor;

        void setBooleanValue(const int valueDefId, const bool value);
        void setIntegerValue(const int valueDefId, const int value);
        void setIntegerValue(const int valueDefId, const unsigned int value);
        void setFloatValue(const int valueDefId, const float value);
        void setStringValue(const int valueDefId, const QString& value);

        bool evaluate(
            const std::shared_ptr<const Model::MapObject>& mapObject, const MapStyleRulesetType ruleset,
            MapStyleEvaluationResult* const outResultStorage = nullptr,
            bool evaluateChildren = true);
        bool evaluate(
            const std::shared_ptr<const MapStyleRule>& singleRule,
            MapStyleEvaluationResult* const outResultStorage = nullptr,
            bool evaluateChildren = true);

        void dump(bool input = true, bool output = true, const QString& prefix = QString()) const;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATOR_H_)
