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
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Nullable.h>

namespace OsmAnd
{
    class ObfMapSectionReader_P;

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
    public:
        ObfMapSectionLevel();
        virtual ~ObfMapSectionLevel();

        uint32_t offset;
        uint32_t length;

        ZoomLevel minZoom;
        ZoomLevel maxZoom;
        AreaI area31;

        uint32_t firstDataBoxInnerOffset;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class OSMAND_CORE_API ObfMapSectionAttributeMapping : public MapObject::AttributeMapping
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapSectionAttributeMapping);
    private:
    protected:
        virtual void registerRequiredMapping(uint32_t& lastUsedEntryId);
    public:
        ObfMapSectionAttributeMapping();
        virtual ~ObfMapSectionAttributeMapping();

        uint32_t tunnelAttributeId;
        uint32_t bridgeAttributeId;

        QSet<uint32_t> positiveLayerAttributeIds;
        QSet<uint32_t> zeroLayerAttributeIds;
        QSet<uint32_t> negativeLayerAttributeIds;

        virtual void registerMapping(const uint32_t id, const QString& tag, const QString& value);
    };

    class ObfMapSectionInfo_P;
    class OSMAND_CORE_API ObfMapSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapSectionInfo);
    private:
        PrivateImplementation<ObfMapSectionInfo_P> _p;
    protected:
    public:
        ObfMapSectionInfo(const std::shared_ptr<const ObfInfo>& container);
        virtual ~ObfMapSectionInfo();

        std::shared_ptr<const ObfMapSectionAttributeMapping> getAttributeMapping() const;

        bool isBasemap;
        bool isBasemapWithCoastlines;
        bool isContourLines;
        QList< Ref<ObfMapSectionLevel> > levels;
        
        const Nullable<LatLon> getCenterLatLon() const;

    friend class OsmAnd::ObfMapSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_INFO_H_)
