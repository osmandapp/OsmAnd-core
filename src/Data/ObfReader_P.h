#ifndef _OSMAND_CORE_OBF_READER_P_H_
#define _OSMAND_CORE_OBF_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QString>
#include <QIODevice>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
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
        mutable std::unique_ptr<gpb::io::ZeroCopyInputStream> _zeroCopyInputStream;

        const std::shared_ptr<QIODevice> _input;
        mutable std::shared_ptr<const ObfInfo> _obfInfo;

        static bool readInfo(const ObfReader_P& reader, std::shared_ptr<const ObfInfo>& info);
    protected:
        ObfReader_P(ObfReader* const owner, const std::shared_ptr<QIODevice>& input);

        mutable std::unique_ptr<gpb::io::CodedInputStream> _codedInputStream;
    public:
        virtual ~ObfReader_P();

        ImplementationInterface<ObfReader> owner;

        bool isOpened() const;
        bool open();
        bool close();

        std::shared_ptr<const ObfInfo> obtainInfo() const;

    friend class OsmAnd::ObfReader;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfAddressSectionReader_P;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfPoiSectionReader_P;
    friend class OsmAnd::ObfTransportSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_READER_P_H_)
