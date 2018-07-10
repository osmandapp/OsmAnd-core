#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "DataCommonTypes.h"
#include "ObfTransportSectionReader.h"
#include "ObfTransportSectionInfo.h"

namespace OsmAnd {

    class ObfReader_P;
    class ObfTransportSectionInfo;
    class IQueryController;

    class ObfTransportSectionReader_P Q_DECL_FINAL
    {
    public:
        typedef ObfTransportSectionReader::TransportStopVisitorFunction TransportStopVisitorFunction;

    private:
        ObfTransportSectionReader_P();
        ~ObfTransportSectionReader_P();
        
        static void initializeStringTable(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfTransportSectionInfo>& section,
            ObfSectionInfo::StringTable* const stringTable);
        
        static void initializeNames(
            ObfSectionInfo::StringTable* const stringTable,
            std::shared_ptr<TransportStop> s);
        
    protected:
        static void read(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfTransportSectionInfo>& section);

        static void readTransportStopsBounds(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfTransportSectionInfo>& section);

        static void searchTransportTreeBounds(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfTransportSectionInfo>& section,
            QList< std::shared_ptr<TransportStop> >& resultOut,
            AreaI& pbbox31,
            const AreaI* const bbox31,
            ObfSectionInfo::StringTable* const stringTable,
            const std::shared_ptr<const IQueryController>& queryController);

        static std::shared_ptr<TransportStop> readTransportStop(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfTransportSectionInfo>& section,
            const uint32_t stopOffset,
            const AreaI& cbbox31,
            const AreaI* const bbox31,
            ObfSectionInfo::StringTable* const stringTable,
            const std::shared_ptr<const IQueryController>& queryController);
    public:
        static void searchTransportStops(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfTransportSectionInfo>& section,
            QList< std::shared_ptr<const TransportStop> >* resultOut = nullptr,
            const AreaI* const bbox31 = nullptr,
            ObfSectionInfo::StringTable* const stringTable = nullptr,
            const TransportStopVisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::ObfTransportSectionReader;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_P_H_)
