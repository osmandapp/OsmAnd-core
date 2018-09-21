#ifndef _OSMAND_CORE_CACHED_OSMAND_INDEXES_H_
#define _OSMAND_CORE_CACHED_OSMAND_INDEXES_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include <QIODevice>
#include <QThread>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include "osmand_index.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/Data/ObfReader.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>

//#define OSMAND_VERIFY_OBF_READER_THREAD 1
#if !defined(OSMAND_VERIFY_OBF_READER_THREAD)
#   define OSMAND_VERIFY_OBF_READER_THREAD 0
#endif // !defined(OSMAND_VERIFY_OBF_READER_THREAD)

namespace OsmAnd
{
    namespace gpb = google::protobuf;

    class OSMAND_CORE_API CachedOsmandIndexes Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CachedOsmandIndexes);

    public:
        static const int VERSION = 2;

    private:
        std::shared_ptr<OBF::OsmAndStoredIndex> storedIndex;
        bool hasChanged;

        void addToCache(const std::shared_ptr<const ObfFile>& file);
        void addRouteSubregion(const std::shared_ptr<const OBF::RoutingPart>& routing, const std::shared_ptr<const OBF::RoutingSubregion>& sub, bool base);
        std::shared_ptr<const ObfInfo> initFileIndex(const std::shared_ptr<const OBF::FileIndex>& found);
        
    public:
        CachedOsmandIndexes();
        virtual ~CachedOsmandIndexes();

        const std::shared_ptr<const ObfFile> getObfFile(const QString& filePath);
        void readFromFile(const QString& filePath, int version);
        void writeToFile(const QString& filePath);
    };
}

#endif // !defined(_OSMAND_CORE_CACHED_OSMAND_INDEXES_H_)
