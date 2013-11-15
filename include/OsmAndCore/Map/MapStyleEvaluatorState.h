#ifndef __MAP_STYLE_EVALUATOR_STATE_H_
#define __MAP_STYLE_EVALUATOR_STATE_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapStyleEvaluatorState_P;
    class OSMAND_CORE_API MapStyleEvaluatorState
    {
        Q_DISABLE_COPY(MapStyleEvaluatorState);
    private:
        const std::unique_ptr<MapStyleEvaluatorState_P> _d;
    protected:
    public:
        MapStyleEvaluatorState();
        virtual ~MapStyleEvaluatorState();

        void clear();

    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_EVALUATOR_P_H_
