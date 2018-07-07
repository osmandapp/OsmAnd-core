#ifndef _OSMAND_CORE_OBF_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QAtomicInt>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/LatLon.h>

namespace OsmAnd
{
    class ObfInfo;

    class ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfSectionInfo);
        
    public:
        typedef QHash<uint32_t, QString> StringTable;

    private:
        static QAtomicInt _nextRuntimeGeneratedId;
    protected:
        ObfSectionInfo(const std::shared_ptr<const ObfInfo>& container);
    public:
        virtual ~ObfSectionInfo();

        const int runtimeGeneratedId;
        const std::weak_ptr<const ObfInfo> container;

        QString name;
        uint32_t length;
        uint32_t offset;

        Nullable<LatLon> calculatedCenter;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_SECTION_INFO_H_)
