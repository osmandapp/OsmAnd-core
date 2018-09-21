#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>
#include <OsmAndCore/CachedOsmandIndexes.h>

namespace OsmAnd {

    class ObfTransportSectionReader_P;
    class ObfReader_P;

    class OSMAND_CORE_API ObfTransportSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfTransportSectionInfo)
        
        struct IndexStringTable
        {
            int fileOffset = 0;
            int length = 0;
        };
        
    private:
    protected:
        ObfTransportSectionInfo(const std::shared_ptr<const ObfInfo>& container);

        AreaI _area31;

        uint32_t _stopsOffset;
        uint32_t _stopsLength;
        Nullable<IndexStringTable> _stringTable;

    public:
        virtual ~ObfTransportSectionInfo();

        const AreaI& area31;
        const uint32_t& stopsOffset;
        const uint32_t& stopsLength;
        const Nullable<IndexStringTable>& stringTable;

        friend class OsmAnd::ObfTransportSectionReader_P;
        friend class OsmAnd::ObfReader_P;
        friend class OsmAnd::CachedOsmandIndexes;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_INFO_H_)
