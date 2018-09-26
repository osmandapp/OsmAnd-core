#ifndef _OSMAND_CORE_CACHED_OSMAND_INDEXES_P_H_
#define _OSMAND_CORE_CACHED_OSMAND_INDEXES_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include "osmand_index.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    namespace gpb = google::protobuf;

    class ObfFile;
    class ObfInfo;
    class ObfRoutingSectionLevelTreeNode;

    class CachedOsmandIndexes;
    class CachedOsmandIndexes_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CachedOsmandIndexes_P);

    private:
        std::shared_ptr<OBF::OsmAndStoredIndex> _storedIndex;
        bool _hasChanged;

        void addToCache(const std::shared_ptr<const ObfFile>& file);
        void addRouteSubregion(OBF::RoutingPart* routing, std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& sub, bool base);
        std::shared_ptr<const ObfInfo> initFileIndex(const std::shared_ptr<const OBF::FileIndex>& found);

    protected:
        CachedOsmandIndexes_P(CachedOsmandIndexes* const owner);
    public:
        virtual ~CachedOsmandIndexes_P();

        ImplementationInterface<CachedOsmandIndexes> owner;
        
        const std::shared_ptr<const ObfFile> getObfFile(const QString& filePath);
        void readFromFile(const QString& filePath, int version);
        void writeToFile(const QString& filePath);
        
    friend class OsmAnd::CachedOsmandIndexes;
    };
}


#endif // !defined(_OSMAND_CORE_CACHED_OSMAND_INDEXES_P_H_)
