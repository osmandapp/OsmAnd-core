#ifndef _OSMAND_CORE_OBF_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QAtomicInt>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class ObfInfo;

    class ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfSectionInfo)
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
    };
}

#endif // !defined(_OSMAND_CORE_OBF_SECTION_INFO_H_)
