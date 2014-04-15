#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMap>
#include <QVariant>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

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

        ImplementationInterface<MapStyleEvaluationResult> owner;

        QMap< int, QVariant > _values;
    public:
        ~MapStyleEvaluationResult_P();

    friend class OsmAnd::MapStyleEvaluationResult;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_P_H_)
