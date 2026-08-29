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

#include <openingHoursParser.h>

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

    struct OSMAND_CORE_API ObfRoutingSectionAttributeMapping : public MapObject::AttributeMapping
    {
    public:
        struct OSMAND_CORE_API RouteTypeCondition Q_DECL_FINAL
        {
            RouteTypeCondition();
            ~RouteTypeCondition();

            QString condition;
            std::shared_ptr<OpeningHoursParser::OpeningHours> hours;
            float floatValue;
        };
        
        class OSMAND_CORE_API RouteTypeRule Q_DECL_FINAL
        {
        private:
            enum {
                Access = 1,
                OneWay = 2,
                HighwayType = 3,
                MaxSpeed = 4,
                Roundabout = 5,
                TrafficSignals = 6,
                RailwayCrossing = 7,
                Lanes = 8,
                Hazard = 9,
            };
            
            QString t;
            QString v;

            int intValue;
            float floatValue;
            int type;
            QList<std::shared_ptr<const RouteTypeCondition>> conditions;
            int forward;
            
            void analyze();
            
        public:
            RouteTypeRule();
            RouteTypeRule(const QString& tag, const QString& value);
            ~RouteTypeRule();

            int isForward() const;
            QString getTag() const;
            QString getValue() const;
            bool roundabout() const;
            int getType() const;
            bool conditional() const;
            int onewayDirection() const;
            float maxSpeed() const;
            int lanes() const;
            QString highwayRoad() const;
        };
        
    public:
        ListMap< RouteTypeRule > routingDecodeMap;

        uint32_t refAttributeId;
        QHash< QStringRef, uint32_t > localizedRefAttributes;
        QHash< uint32_t, QStringRef> localizedRefAttributeIds;
        QSet< uint32_t > refAttributeIds;

        ObfRoutingSectionAttributeMapping();
        virtual ~ObfRoutingSectionAttributeMapping();
        
        virtual void registerMapping(const uint32_t id, const QString& tag, const QString& value);
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

        std::shared_ptr<const ObfRoutingSectionAttributeMapping> getAttributeMapping() const;

        AreaI area31;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_)
