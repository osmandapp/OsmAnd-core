#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QVector>
#include <QHash>
#include <QSet>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>
#include <OsmAndCore/Data/MapObject.h>

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;
    class Road;

    class ObfRoutingSectionLevelTreeNode;
    class ObfRoutingSectionLevel_P;
    class OSMAND_CORE_API ObfRoutingSectionLevel
    {
        Q_DISABLE_COPY_AND_MOVE(ObfRoutingSectionLevel);
    private:
        PrivateImplementation<ObfRoutingSectionLevel_P> _p;
    protected:
    public:
        ObfRoutingSectionLevel(const RoutingDataLevel dataLevel);
        virtual ~ObfRoutingSectionLevel();

        const RoutingDataLevel dataLevel;
        const QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> > &rootNodes;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };

    class OSMAND_CORE_API ObfRoutingSectionLevelTreeNode Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfRoutingSectionLevelTreeNode);
    private:
    protected:
    public:
        ObfRoutingSectionLevelTreeNode();
        ~ObfRoutingSectionLevelTreeNode();

        uint32_t length;
        uint32_t offset;

        AreaI area31;

        bool hasChildrenDataBoxes;
        uint32_t firstDataBoxInnerOffset;
        uint32_t dataOffset;
    };

    struct OSMAND_CORE_API ObfRoutingSectionEncodingDecodingRules : public MapObject::EncodingDecodingRules
    {
        ObfRoutingSectionEncodingDecodingRules();
        virtual ~ObfRoutingSectionEncodingDecodingRules();
    };

    class ObfRoutingSectionInfo_P;
    class OSMAND_CORE_API ObfRoutingSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfRoutingSectionInfo)
    private:
        PrivateImplementation<ObfRoutingSectionInfo_P> _p;
    protected:
    public:
        ObfRoutingSectionInfo(const std::shared_ptr<const ObfInfo>& container);
        virtual ~ObfRoutingSectionInfo();

        std::shared_ptr<const ObfRoutingSectionEncodingDecodingRules> getEncodingDecodingRules() const;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_)
