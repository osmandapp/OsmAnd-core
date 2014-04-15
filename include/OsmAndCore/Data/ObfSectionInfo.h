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
    class ObfReader;
    class ObfReader_P;

    class ObfSectionInfo
    {
    private:
        static QAtomicInt _nextRuntimeGeneratedId;
    protected:
        ObfSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        QString _name;
        uint32_t _length;
        uint32_t _offset;
    public:
        virtual ~ObfSectionInfo();

        const QString& name;
        const uint32_t &length;
        const uint32_t &offset;

        const int runtimeGeneratedId;
        
        std::weak_ptr<ObfInfo> owner;

    friend class OsmAnd::ObfReader;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_SECTION_INFO_H_)
