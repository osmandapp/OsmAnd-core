#ifndef _OSMAND_CORE_MAP_OBJECT_H_
#define _OSMAND_CORE_MAP_OBJECT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QVector>
#include <QString>
#include <QHash>
#include <QSet>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapObject
    {
        Q_DISABLE_COPY_AND_MOVE(MapObject);
    public:
        class OSMAND_CORE_API EncodingDecodingRules
        {
            Q_DISABLE_COPY_AND_MOVE(EncodingDecodingRules);
        public:
            struct OSMAND_CORE_API DecodingRule Q_DECL_FINAL
            {
                DecodingRule();
                ~DecodingRule();

                QString tag;
                QString value;

                QString toString() const;
            };

        private:
        protected:
            virtual void createRequiredRules(uint32_t& lastUsedRuleId);
        public:
            EncodingDecodingRules();
            virtual ~EncodingDecodingRules();

            // All rules
            QHash< QString, QHash<QString, uint32_t> > encodingRuleIds;
            QHash< uint32_t, DecodingRule > decodingRules;

            // Quick-access rules
            uint32_t name_encodingRuleId;
            QHash< QString, uint32_t > localizedName_encodingRuleIds;
            QHash< uint32_t, QString> localizedName_decodingRules;
            QSet< uint32_t > namesRuleId;
            uint32_t ref_encodingRuleId;
            uint32_t naturalCoastline_encodingRuleId;
            uint32_t naturalLand_encodingRuleId;
            uint32_t naturalCoastlineBroken_encodingRuleId;
            uint32_t naturalCoastlineLine_encodingRuleId;
            uint32_t oneway_encodingRuleId;
            uint32_t onewayReverse_encodingRuleId;
            uint32_t layerLowest_encodingRuleId;

            virtual uint32_t addRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue);
            void verifyRequiredRulesExist();
        };

        typedef uint64_t SharingKey;
        typedef uint64_t SortingKey;

        enum class LayerType
        {
            Negative = -1,
            Zero = 0,
            Positive = +1
        };

        // Map objects comparator using sorting key:
        // - If key is available, sort by key in ascending order in case keys differ. Otherwise sort by pointer
        // - If key is unavailable, sort by pointer
        // - If key vs no-key is sorted, no-key is always goes "greater" than key
        //NOTE: Symbol ordering should take into account ordering of primitives actually (in cases that apply)
        struct OSMAND_CORE_API Comparator Q_DECL_FINAL
        {
            Comparator();

            bool operator()(const std::shared_ptr<const MapObject>& l, const std::shared_ptr<const MapObject>& r) const;
        };

    private:
    protected:
    public:
        MapObject();
        virtual ~MapObject();

        // General information
        virtual bool obtainSharingKey(SharingKey& outKey) const;
        virtual bool obtainSortingKey(SortingKey& outKey) const;
        std::shared_ptr<const EncodingDecodingRules> encodingDecodingRules;
        virtual ZoomLevel getMinZoomLevel() const;
        virtual ZoomLevel getMaxZoomLevel() const;
        virtual QString toString() const;
        
        // Geometry information
        bool isArea;
        QVector< PointI > points31;
        QList< QVector< PointI > > innerPolygonsPoints31;
        AreaI bbox31;
        virtual bool isClosedFigure(bool checkInner = false) const;
        virtual void computeBBox31();
        virtual bool intersectedOrContainedBy(const AreaI& area) const;

        // Rules
        QVector< uint32_t > typesRuleIds;
        QVector< uint32_t > additionalTypesRuleIds;
        virtual bool containsType(const uint32_t typeRuleId, bool checkAdditional = false) const;
        virtual bool containsTypeSlow(const QString& tag, const QString& value, bool checkAdditional = false) const;
        virtual bool containsTagSlow(const QString& tag, bool checkAdditional = false) const;
        virtual bool obtainTagValueByTypeRuleIndex(
            const uint32_t typeRuleIndex,
            QString& outTag,
            QString& outValue,
            bool checkAdditional = false) const;

        // Layers
        virtual LayerType getLayerType() const;

        // Captions
        QHash<uint32_t, QString> captions;
        QList<uint32_t> captionsOrder;
        virtual QString getCaptionInNativeLanguage() const;
        virtual QString getCaptionInLanguage(const QString& lang) const;

        // Default encoding-decoding rules
        static std::shared_ptr<const EncodingDecodingRules> defaultEncodingDecodingRules;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECT_H_)
