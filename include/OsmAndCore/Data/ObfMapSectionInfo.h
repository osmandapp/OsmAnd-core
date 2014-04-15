#ifndef _OSMAND_CORE_OBF_MAP_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>
#include <QMap>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd
{
    class ObfMapSectionReader_P;
    class ObfReader_P;

    class ObfMapSectionLevel_P;
    class OSMAND_CORE_API ObfMapSectionLevel
    {
        Q_DISABLE_COPY(ObfMapSectionLevel);
    private:
        PrivateImplementation<ObfMapSectionLevel_P> _p;
    protected:
        ObfMapSectionLevel();

        uint32_t _offset;
        uint32_t _length;
        ZoomLevel _minZoom;
        ZoomLevel _maxZoom;
        AreaI _area31;

        uint32_t _boxesInnerOffset;
    public:
        virtual ~ObfMapSectionLevel();

        const uint32_t& offset;
        const uint32_t& length;
        
        const ZoomLevel& minZoom;
        const ZoomLevel& maxZoom;
        const AreaI& area31;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    struct ObfMapSectionDecodingRule
    {
        uint32_t type;

        QString tag;
        QString value;
    };

    struct OSMAND_CORE_API ObfMapSectionDecodingEncodingRules
    {
        ObfMapSectionDecodingEncodingRules();

        QHash< QString, QHash<QString, uint32_t> > encodingRuleIds;
        QHash< uint32_t, ObfMapSectionDecodingRule > decodingRules;
        uint32_t name_encodingRuleId;
        uint32_t latinName_encodingRuleId;
        uint32_t ref_encodingRuleId;
        uint32_t naturalCoastline_encodingRuleId;
        uint32_t naturalLand_encodingRuleId;
        uint32_t naturalCoastlineBroken_encodingRuleId;
        uint32_t naturalCoastlineLine_encodingRuleId;
        uint32_t highway_encodingRuleId;
        uint32_t oneway_encodingRuleId;
        uint32_t onewayReverse_encodingRuleId;
        uint32_t tunnel_encodingRuleId;
        uint32_t bridge_encodingRuleId;
        uint32_t layerLowest_encodingRuleId;

        QSet<uint32_t> positiveLayers_encodingRuleIds;
        QSet<uint32_t> zeroLayers_encodingRuleIds;
        QSet<uint32_t> negativeLayers_encodingRuleIds;

        void createRule(const uint32_t ruleType, const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue);
        void createMissingRules();
    };

    class ObfMapSectionInfo_P;
    class OSMAND_CORE_API ObfMapSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfMapSectionInfo);
    private:
        PrivateImplementation<ObfMapSectionInfo_P> _p;
    protected:
        ObfMapSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        bool _isBasemap;
        QList< std::shared_ptr<const ObfMapSectionLevel> > _levels;
    public:
        ObfMapSectionInfo();
        virtual ~ObfMapSectionInfo();

        const bool& isBasemap;
        const QList< std::shared_ptr<const ObfMapSectionLevel> >& levels;

        const std::shared_ptr<const ObfMapSectionDecodingEncodingRules>& encodingDecodingRules;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_INFO_H_)
