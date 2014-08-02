#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QHash>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;
    class ObfReader_P;
    namespace Model
    {
        class Road;
    }

    struct OSMAND_CORE_API ObfRoutingSectionDecodingRule
    {
        QString tag;
        QString value;
    };

    struct OSMAND_CORE_API ObfRoutingSectionEncodingDecodingRules
    {
        ObfRoutingSectionEncodingDecodingRules();

        QHash< QString, QHash<QString, uint32_t> > encodingRuleIds;
        QHash< uint32_t, ObfRoutingSectionDecodingRule > decodingRules;
        uint32_t name_encodingRuleId;
        QHash< QString, uint32_t > localizedName_encodingRuleIds;
        QSet< uint32_t > namesRuleId;

        void createRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue);
        void createMissingRules();
    };

    class ObfRoutingSectionInfo_P;
    class OSMAND_CORE_API ObfRoutingSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfRoutingSectionInfo)
    private:
        PrivateImplementation<ObfRoutingSectionInfo_P> _p;
    protected:
        ObfRoutingSectionInfo(const std::weak_ptr<ObfInfo>& owner);
    public:
        virtual ~ObfRoutingSectionInfo();

        const std::shared_ptr<const ObfRoutingSectionEncodingDecodingRules>& encodingDecodingRules;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };

    class ObfRoutingSectionLevelTreeNode;
    class ObfRoutingSectionLevel_P;
    class OSMAND_CORE_API ObfRoutingSectionLevel
    {
        Q_DISABLE_COPY(ObfRoutingSectionLevel);
    private:
        PrivateImplementation<ObfRoutingSectionLevel_P> _p;
    protected:
        ObfRoutingSectionLevel(const RoutingDataLevel dataLevel);
    public:
        virtual ~ObfRoutingSectionLevel();

        const RoutingDataLevel dataLevel;
        const QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> > &rootNodes;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };

    class OSMAND_CORE_API ObfRoutingSectionLevelTreeNode Q_DECL_FINAL
    {
    private:
    protected:
        ObfRoutingSectionLevelTreeNode();
    public:
        ~ObfRoutingSectionLevelTreeNode();

        uint32_t length;
        uint32_t offset;

        AreaI area31;

        uint32_t childrenRelativeOffset;
        uint32_t dataOffset;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_)
