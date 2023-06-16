#ifndef _OSMAND_CORE_OBF_READER_P_H_
#define _OSMAND_CORE_OBF_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include <QIODevice>
#include <QThread>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

//#define OSMAND_VERIFY_OBF_READER_THREAD 1
#if !defined(OSMAND_VERIFY_OBF_READER_THREAD)
#   define OSMAND_VERIFY_OBF_READER_THREAD 0
#endif // !defined(OSMAND_VERIFY_OBF_READER_THREAD)

namespace OsmAnd
{
    namespace gpb = google::protobuf;

    static const int TRANSPORT_STOP_ZOOM = 24;

    class ObfInfo;

    class ObfReader;
    class ObfReader_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfReader_P);

    private:
        const std::shared_ptr<QIODevice> _input;
        std::shared_ptr<gpb::io::ZeroCopyInputStream> _zeroCopyInputStream;
        std::shared_ptr<gpb::io::CodedInputStream> _codedInputStream;

        mutable std::shared_ptr<const ObfInfo> _obfInfo;
        static bool readInfo(const ObfReader_P& reader, std::shared_ptr<ObfInfo>& info);
        static bool readOsmAndOwner(gpb::io::CodedInputStream* cis, const std::shared_ptr<ObfInfo> info);

#if OSMAND_VERIFY_OBF_READER_THREAD
        const Qt::HANDLE _threadId;
#endif // OSMAND_VERIFY_OBF_READER_THREAD
    protected:
        ObfReader_P(ObfReader* const owner, const std::shared_ptr<QIODevice>& input);
    public:
        virtual ~ObfReader_P();

        ImplementationInterface<ObfReader> owner;

        bool isOpened() const;
        bool open();
        bool close();

        std::shared_ptr<const ObfInfo> obtainInfo() const;

        std::shared_ptr<gpb::io::CodedInputStream> getCodedInputStream() const;

    friend class OsmAnd::ObfReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_READER_P_H_)
