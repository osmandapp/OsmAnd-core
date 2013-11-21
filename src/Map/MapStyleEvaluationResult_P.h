#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;
    
    class MapStyleEvaluationResult;
    class MapStyleEvaluationResult_P
    {
    public:
        union EvaluatedValue_POD
        {
            inline EvaluatedValue_POD()
            {
                asUInt = 0;
            }

            float asFloat;
            int32_t asInt;
            uint32_t asUInt;
        };
    private:
    protected:
        MapStyleEvaluationResult_P(MapStyleEvaluationResult* const owner);

        MapStyleEvaluationResult* const owner;

        QMap< int, EvaluatedValue_POD > _podValues;
        QMap< int, QString > _stringValues;
    public:
        ~MapStyleEvaluationResult_P();

    friend class OsmAnd::MapStyleEvaluationResult;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_
