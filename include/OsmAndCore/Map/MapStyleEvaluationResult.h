#ifndef _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_
#define _OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_

#include <OsmAndCore/stdlib_common.h>
#include <cstdarg>
#include <utility>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>
#include <QVariant>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/ResolvedMapStyle.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapStyleEvaluationResult Q_DECL_FINAL
    {
    private:
    protected:
        QVector<QVariant> _storage;

    public:
        MapStyleEvaluationResult(const int size = 0);
        MapStyleEvaluationResult(const MapStyleEvaluationResult& that);
#ifdef Q_COMPILER_RVALUE_REFS
        MapStyleEvaluationResult(MapStyleEvaluationResult&& that);
#endif // Q_COMPILER_RVALUE_REFS
        ~MapStyleEvaluationResult();

        MapStyleEvaluationResult& operator=(const MapStyleEvaluationResult& that);
#ifdef Q_COMPILER_RVALUE_REFS
        MapStyleEvaluationResult& operator=(MapStyleEvaluationResult&& that);
#endif // Q_COMPILER_RVALUE_REFS

        bool contains(const ResolvedMapStyle::ValueDefinitionId valueDefId) const;

        void setValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const QVariant& value);
        void setBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const bool& value);
        void setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const int& value);
        void setIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const unsigned int& value);
        void setFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const float& value);
        void setStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, const QString& value);
    
        bool getValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, QVariant& outValue) const;
        bool getBooleanValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, bool& outValue) const;
        bool getIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, int& outValue) const;
        bool getIntegerValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, unsigned int& outValue) const;
        bool getFloatValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, float& outValue) const;
        bool getStringValue(const ResolvedMapStyle::ValueDefinitionId valueDefId, QString& outValue) const;

        void reset();
        void clear();
        bool isEmpty() const;

        typedef std::pair<ResolvedMapStyle::ValueDefinitionId, QVariant> PackedResultEntry;
        typedef QList<PackedResultEntry> PackedResult;
        void pack(PackedResult& packedResult) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_EVALUATION_RESULT_H_)
