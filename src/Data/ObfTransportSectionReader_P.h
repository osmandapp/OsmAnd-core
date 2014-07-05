#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"

namespace OsmAnd {

    class ObfReader_P;
    class ObfTransportSectionInfo;
    class IQueryController;

    class ObfTransportSectionReader_P Q_DECL_FINAL
    {
    private:
        ObfTransportSectionReader_P();
        ~ObfTransportSectionReader_P();
    protected:
        static void read(const ObfReader_P& reader, const std::shared_ptr<ObfTransportSectionInfo>& section);

        static void readTransportStopsBounds(const ObfReader_P& reader, const std::shared_ptr<ObfTransportSectionInfo>& section);

    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_)
