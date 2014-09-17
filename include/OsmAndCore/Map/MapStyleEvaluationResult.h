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
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/ResolvedMapStyle.h>

namespace OsmAnd
{
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

        QHash<ResolvedMapStyle::ValueDefinitionId, QVariant> values;

        bool contains(const ResolvedMapStyle::ValueDefinitionId valueDefId) const;
    
        bool getBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, bool& value) const;
        bool getIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, int& value) const;
        bool getIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, unsigned int& value) const;
        bool getFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, float& value) const;
        bool getStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, QString& value) const;

        void clear();
        bool isEmpty() const;

        typedef std::pair<ResolvedMapStyle::ValueDefinitionId, QVariant> PackedResultEntry;
        typedef QList<PackedResultEntry> PackedResult;
        void pack(PackedResult& packedResult);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_)
