#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>

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

    struct OSMAND_CORE_API ObfRoutingSectionDecodingRules
    {
        enum class RuleType : uint32_t
        {
            Access = 1,
            OneWay = 2,
            Highway = 3,
            Maxspeed = 4,
            Roundabout = 5,
            TrafficSignals = 6,
            RailwayCrossing = 7,
            Lanes = 8,
        };

        struct OSMAND_CORE_API DecodingRule
        {
            uint32_t id;
            RuleType type;
            
            QString tag;
            QString value;
            
            union
            {
                int32_t asSignedInt;
                uint32_t asUnsignedInt;
                float asFloat;
            } parsedValue;
        };

        QList< std::shared_ptr<const DecodingRule> > decodingRules;
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

        const std::shared_ptr<const ObfRoutingSectionDecodingRules>& decodingRules;

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

        AreaI bbox31;

        uint32_t childrenRelativeOffset;
        uint32_t dataOffset;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_)
