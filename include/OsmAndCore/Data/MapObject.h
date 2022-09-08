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
#include <OsmAndCore/ListMap.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapObject
    {
        Q_DISABLE_COPY_AND_MOVE(MapObject);
    public:
        class OSMAND_CORE_API AttributeMapping
        {
            Q_DISABLE_COPY_AND_MOVE(AttributeMapping);
        public:
            struct OSMAND_CORE_API TagValue Q_DECL_FINAL
            {
                TagValue();
                ~TagValue();

                QString tag;
                QString value;

                QString toString() const;
            };

        private:
        protected:
            virtual void registerRequiredMapping(uint32_t& lastUsedEntryId);
        public:
            AttributeMapping();
            virtual ~AttributeMapping();

            ListMap< TagValue > decodeMap;
            QHash< QStringRef, QHash<QStringRef, uint32_t> > encodeMap;

            uint32_t nativeNameAttributeId;
            QHash< QStringRef, uint32_t > localizedNameAttributes;
            QHash< uint32_t, QStringRef> localizedNameAttributeIds;
            QSet< uint32_t > nameAttributeIds;
            uint32_t enNameAttributeId;
            uint32_t refAttributeId;
            uint32_t naturalCoastlineAttributeId;
            uint32_t naturalLandAttributeId;
            uint32_t naturalCoastlineBrokenAttributeId;
            uint32_t naturalCoastlineLineAttributeId;
            uint32_t onewayAttributeId;
            uint32_t onewayReverseAttributeId;
            uint32_t layerLowestAttributeId;

            virtual void registerMapping(const uint32_t id, const QString& tag, const QString& value);
            void verifyRequiredMappingRegistered();

            bool encodeTagValue(
                const QStringRef& tagRef,
                const QStringRef& valueRef,
                uint32_t* const pOutAttributeId = nullptr) const;
            bool encodeTagValue(
                const QString& tag,
                const QString& value,
                uint32_t* const pOutAttributeId = nullptr) const;
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
        std::shared_ptr<const AttributeMapping> attributeMapping;
        virtual ZoomLevel getMinZoomLevel() const;
        virtual ZoomLevel getMaxZoomLevel() const;
        virtual QString toString() const;
        
        // Geometry information
        bool isArea;
        bool isCoastline;
        QVector< PointI > points31;
        QList< QVector< PointI > > innerPolygonsPoints31;
        AreaI bbox31;
        virtual bool isClosedFigure(bool checkInner = false) const;
        virtual void computeBBox31();
        virtual bool intersectedOrContainedBy(const AreaI& area) const;

        // Attributes
        QVector< uint32_t > attributeIds;
        QVector< uint32_t > additionalAttributeIds;
        bool containsAttribute(const uint32_t attributeId, const bool checkAdditional = false) const;
        bool containsAttribute(const QString& tag, const QString& value, const bool checkAdditional = false) const;
        bool containsAttribute(
            const QStringRef& tagRef,
            const QStringRef& valueRef,
            const bool checkAdditional = false) const;
        bool containsTag(const QString& tag, const bool checkAdditional = false) const;
        bool containsTag(const QStringRef& tagRef, const bool checkAdditional = false) const;
        const AttributeMapping::TagValue* resolveAttributeByIndex(const uint32_t index, const bool additional = false) const;
        virtual QHash<QString, QString> getResolvedAttributes() const;

        // Layers
        virtual LayerType getLayerType() const;

        // Captions
        QHash<uint32_t, QString> captions;
        QList<uint32_t> captionsOrder;
        virtual QString getCaptionInNativeLanguage() const;
        virtual QString getCaptionInLanguage(const QString& lang) const;
        virtual QHash<QString, QString> getCaptionsInAllLanguages() const;
        virtual QString getName(const QString lang, bool transliterate) const;
        virtual QString debugInfo(long id, bool all) const;

        // Default encoding-decoding rules
        static std::shared_ptr<const AttributeMapping> defaultAttributeMapping;
        int32_t labelX;
        int32_t labelY;
        bool isLabelCoordinatesSpecified() const;
        int32_t getLabelCoordinateX() const;
        int32_t getLabelCoordinateY() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECT_H_)
