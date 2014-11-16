#ifndef _OSMAND_CORE_OBF_MAP_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QHash>
#include <QMap>
#include <QSet>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Ref.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>
#include <OsmAndCore/Data/MapObject.h>

namespace OsmAnd
{
    class ObfMapSectionReader_P;
    class ObfReader_P;

    class ObfMapSectionLevel_P;
    class OSMAND_CORE_API ObfMapSectionLevel
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapSectionLevel);
    public:
        enum {
            MaxBasemapZoomLevel = ZoomLevel11
        };

    private:
        PrivateImplementation<ObfMapSectionLevel_P> _p;
    protected:
        ObfMapSectionLevel();
    public:
        virtual ~ObfMapSectionLevel();

        uint32_t offset;
        uint32_t length;

        ZoomLevel minZoom;
        ZoomLevel maxZoom;
        AreaI area31;

        uint32_t firstDataBoxInnerOffset;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class OSMAND_CORE_API ObfMapSectionDecodingEncodingRules : public MapObject::EncodingDecodingRules
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapSectionDecodingEncodingRules);
    private:
    protected:
        virtual void createRequiredRules(uint32_t& lastUsedRuleId);
    public:
        ObfMapSectionDecodingEncodingRules();
        virtual ~ObfMapSectionDecodingEncodingRules();

        // Quick-access rules
        uint32_t tunnel_encodingRuleId;
        uint32_t bridge_encodingRuleId;

        QSet<uint32_t> positiveLayers_encodingRuleIds;
        QSet<uint32_t> zeroLayers_encodingRuleIds;
        QSet<uint32_t> negativeLayers_encodingRuleIds;

        virtual uint32_t addRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue);
    };

    class ObfMapSectionInfo_P;
    class OSMAND_CORE_API ObfMapSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapSectionInfo);
    private:
        PrivateImplementation<ObfMapSectionInfo_P> _p;
    protected:
        ObfMapSectionInfo(const std::weak_ptr<ObfInfo>& owner);
    public:
        virtual ~ObfMapSectionInfo();

        std::shared_ptr<const ObfMapSectionDecodingEncodingRules> getEncodingDecodingRules() const;

        bool isBasemap;
        QList< Ref<ObfMapSectionLevel> > levels;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_INFO_H_)
