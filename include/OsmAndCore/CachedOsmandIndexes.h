#ifndef _OSMAND_CORE_CACHED_OSMAND_INDEXES_H_
#define _OSMAND_CORE_CACHED_OSMAND_INDEXES_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include "restore_internal_warnings.h"

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>

namespace OsmAnd
{
    class IObfsCollection;

    class CachedOsmandIndexes_P;
    class OSMAND_CORE_API CachedOsmandIndexes Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CachedOsmandIndexes);

    public:
        static const int VERSION = 2;

    private:
        PrivateImplementation<CachedOsmandIndexes_P> _p;
        
    public:
        CachedOsmandIndexes();
        virtual ~CachedOsmandIndexes();

        const std::shared_ptr<ObfFile> getObfFile(const QString& filePath);
        void readFromFile(const QString& filePath, int version);
        void writeToFile(const QString& filePath);
    };
}

#endif // !defined(_OSMAND_CORE_CACHED_OSMAND_INDEXES_H_)
