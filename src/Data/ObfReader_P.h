#ifndef _OSMAND_CORE_OBF_READER_P_H_
#define _OSMAND_CORE_OBF_READER_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <OsmAndCore.h>

class QIODevice;

namespace OsmAnd {

    namespace gpb = google::protobuf;

    class ObfInfo;

    class ObfMapSectionReader_P;
    class ObfAddressSectionReader_P;
    class ObfRoutingSectionReader_P;
    class ObfPoiSectionReader_P;
    class ObfTransportSectionReader_P;

    class ObfReader;
    class ObfReader_P
    {
    private:
    protected:
        ObfReader_P(ObfReader* owner);

        ObfReader* const owner;
        std::unique_ptr<gpb::io::CodedInputStream> _codedInputStream;
        std::unique_ptr<gpb::io::ZeroCopyInputStream> _zeroCopyInputStream;

        std::shared_ptr<QIODevice> _input;
        std::shared_ptr<ObfInfo> _obfInfo;

        static void readInfo(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfInfo>& info);
    public:
        virtual ~ObfReader_P();

    friend class OsmAnd::ObfReader;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfAddressSectionReader_P;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfPoiSectionReader_P;
    friend class OsmAnd::ObfTransportSectionReader_P;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_READER_P_H_)
