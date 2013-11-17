#ifndef __MAP_STYLE_EVALUATOR_STATE_P_H_
#define __MAP_STYLE_EVALUATOR_STATE_P_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>

#include <OsmAndCore.h>
#include <MapStyleValue.h>

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;

    class MapStyleEvaluatorState;
    class MapStyleEvaluatorState_P
    {
    private:
    protected:
        MapStyleEvaluatorState_P(MapStyleEvaluatorState* const owner);

        MapStyleEvaluatorState* const owner;

        QMap< int, MapStyleValue > _values;
    public:
        ~MapStyleEvaluatorState_P();

    friend class OsmAnd::MapStyleEvaluatorState;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_EVALUATOR_P_H_
