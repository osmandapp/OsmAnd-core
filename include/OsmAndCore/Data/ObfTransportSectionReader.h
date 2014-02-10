#ifndef _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_H_
#define _OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfTransportSectionInfo;
    class IQueryController;

    class OSMAND_CORE_API ObfTransportSectionReader
    {
    private:
        ObfTransportSectionReader();
        ~ObfTransportSectionReader();
    protected:
    public:
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_TRANSPORT_SECTION_READER_H_)
