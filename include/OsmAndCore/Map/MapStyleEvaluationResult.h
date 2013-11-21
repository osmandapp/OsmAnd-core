#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapStyleEvaluationResult_P;
    class OSMAND_CORE_API MapStyleEvaluationResult
    {
        Q_DISABLE_COPY(MapStyleEvaluationResult);
    private:
        const std::unique_ptr<MapStyleEvaluationResult_P> _d;
    protected:
    public:
        MapStyleEvaluationResult();
        virtual ~MapStyleEvaluationResult();

        bool getBooleanValue(const int valueDefId, bool& value) const;
        bool getIntegerValue(const int valueDefId, int& value) const;
        bool getIntegerValue(const int valueDefId, unsigned int& value) const;
        bool getFloatValue(const int valueDefId, float& value) const;
        bool getStringValue(const int valueDefId, QString& value) const;

        void clear();

    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_
