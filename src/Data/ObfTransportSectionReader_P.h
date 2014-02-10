#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader_P;
    class ObfTransportSectionInfo;
    class IQueryController;

    class OSMAND_CORE_API ObfTransportSectionReader_P
    {
    private:
        ObfTransportSectionReader_P();
        ~ObfTransportSectionReader_P();
    protected:
        static void read(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfTransportSectionInfo>& section);

        static void readTransportStopsBounds(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfTransportSectionInfo>& section);

    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_)
