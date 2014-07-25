#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_

#include <OsmAndCore/stdlib_common.h>
#include <cstdarg>
#include <utility>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVariant>
#include <QHash>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    struct OSMAND_CORE_API MapStyleEvaluationResult Q_DECL_FINAL
    {
        MapStyleEvaluationResult();
        MapStyleEvaluationResult(const MapStyleEvaluationResult& that);
#ifdef Q_COMPILER_RVALUE_REFS
        MapStyleEvaluationResult(MapStyleEvaluationResult&& that);
#endif // Q_COMPILER_RVALUE_REFS
        ~MapStyleEvaluationResult();

        MapStyleEvaluationResult& operator=(const MapStyleEvaluationResult& that);
#ifdef Q_COMPILER_RVALUE_REFS
        MapStyleEvaluationResult& operator=(MapStyleEvaluationResult&& that);
#endif // Q_COMPILER_RVALUE_REFS

        QHash<int, QVariant> values;
    
        bool getBooleanValue(const int valueDefId, bool& value) const;
        bool getIntegerValue(const int valueDefId, int& value) const;
        bool getIntegerValue(const int valueDefId, unsigned int& value) const;
        bool getFloatValue(const int valueDefId, float& value) const;
        bool getStringValue(const int valueDefId, QString& value) const;

        void clear();
        bool isEmpty() const;

        typedef std::pair<int, QVariant> PackedResultEntry;
        typedef QList<PackedResultEntry> PackedResult;
        void pack(PackedResult& packedResult);

    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_)
