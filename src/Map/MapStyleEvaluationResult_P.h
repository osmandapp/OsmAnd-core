#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>
#include <QVariant>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;
    
    class MapStyleEvaluationResult;
    class MapStyleEvaluationResult_P
    {
    private:
    protected:
        MapStyleEvaluationResult_P(MapStyleEvaluationResult* const owner);

        MapStyleEvaluationResult* const owner;

        QMap< int, QVariant > _values;
    public:
        ~MapStyleEvaluationResult_P();

    friend class OsmAnd::MapStyleEvaluationResult;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_)
