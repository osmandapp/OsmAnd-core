#ifndef _OSMAND_CORE_OBF_READER_H_
#define _OSMAND_CORE_OBF_READER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QIODevice>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <google/protobuf/io/coded_stream.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/ObfInfo.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    namespace gpb = google::obf_protobuf;

    class ObfFile;

    class ObfMapSectionReader;
    class ObfAddressSectionReader;
    class ObfRoutingSectionReader;
    class ObfPoiSectionReader;
    class ObfTransportSectionReader;

    class ObfReader_P;
    class OSMAND_CORE_API ObfReader
    {
        Q_DISABLE_COPY_AND_MOVE(ObfReader)
    private:
        PrivateImplementation<ObfReader_P> _p;
    protected:
    public:
        ObfReader(const std::shared_ptr<const ObfFile>& obfFile);
        ObfReader(const std::shared_ptr<QIODevice>& input);
        virtual ~ObfReader();

        const std::shared_ptr<const ObfFile> obfFile;

        bool isOpened() const;
        bool open();
        bool close();

        std::shared_ptr<const ObfInfo> obtainInfo() const;

        std::shared_ptr<gpb::io::CodedInputStream> getCodedInputStream() const;

    friend class OsmAnd::ObfMapSectionReader;
    friend class OsmAnd::ObfAddressSectionReader;
    friend class OsmAnd::ObfRoutingSectionReader;
    friend class OsmAnd::ObfPoiSectionReader;
    friend class OsmAnd::ObfTransportSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_READER_H_)
