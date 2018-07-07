#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfReader;
    class ObfTransportSectionInfo;
    class TransportStop;
    class IQueryController;

    class OSMAND_CORE_API ObfTransportSectionReader
    {
    public:
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::TransportStop>& transportStop)> TransportStopVisitorFunction;

    private:
        ObfTransportSectionReader();
        ~ObfTransportSectionReader();
    protected:
    public:
        static void searchTransportStops(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfTransportSectionInfo>& section,
            QList< std::shared_ptr<const TransportStop> >* resultOut = nullptr,
            const AreaI* const bbox31 = nullptr,
            ObfSectionInfo::StringTable* const stringTable = nullptr,
            const TransportStopVisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_H_)
