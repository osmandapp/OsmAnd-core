#ifndef _OSMAND_CORE_OBF_POI_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_POI_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfPoiSectionReader_P;
    class ObfReader_P;

    class OSMAND_CORE_API ObfPoiSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfPoiSectionInfo)
    private:
    protected:
        ObfPoiSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        AreaI _area31;
    public:
        virtual ~ObfPoiSectionInfo();

        const AreaI& area31;

        friend class OsmAnd::ObfPoiSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_POI_SECTION_INFO_H_)
