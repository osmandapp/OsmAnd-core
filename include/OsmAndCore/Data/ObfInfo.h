#ifndef _OSMAND_CORE_OBF_INFO_H_
#define _OSMAND_CORE_OBF_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Ref.h>

namespace OsmAnd
{
    class ObfMapSectionInfo;
    class ObfAddressSectionInfo;
    class ObfRoutingSectionInfo;
    class ObfPoiSectionInfo;
    class ObfTransportSectionInfo;

    class OSMAND_CORE_API ObfInfo Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfInfo)

    private:
    protected:
    public:
        ObfInfo();
        virtual ~ObfInfo();

        int version;
        uint64_t creationTimestamp;
        bool isBasemap;
        bool isBasemapWithCoastlines;

        QList< Ref<ObfMapSectionInfo> > mapSections;
        QList< Ref<ObfAddressSectionInfo> > addressSections;
        QList< Ref<ObfRoutingSectionInfo> > routingSections;
        QList< Ref<ObfPoiSectionInfo> > poiSections;
        QList< Ref<ObfTransportSectionInfo> > transportSections;

        bool containsDataFor(
            const AreaI* const pBbox31,
            const ZoomLevel minZoomLevel,
            const ZoomLevel maxZoomLevel,
            const ObfDataTypesMask desiredDataTypes) const;

        bool containsPOIFor(const AreaI* const pBbox31) const;

    };
}

#endif // !defined(_OSMAND_CORE_OBF_INFO_H_)
