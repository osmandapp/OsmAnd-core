#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfTransportSectionReader_P;
    class ObfReader_P;

    class OSMAND_CORE_API ObfTransportSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfTransportSectionInfo)
    private:
    protected:
        ObfTransportSectionInfo(const std::shared_ptr<const ObfInfo>& container);

        AreaI _area24;

        uint32_t _stopsOffset;
        uint32_t _stopsLength;
    public:
        virtual ~ObfTransportSectionInfo();

        const AreaI& area24;

        friend class OsmAnd::ObfTransportSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_INFO_H_)
