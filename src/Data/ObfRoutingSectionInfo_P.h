#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_P_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QString>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;
    namespace Model
    {
        enum class RoadDirection : int32_t;
        class Road;
    }
    class RoutePlanner;
    class RoutePlannerContext;
    class RoutingRulesetContext;

    class ObfRoutingSectionInfo;
    class ObfRoutingSectionInfo_P
    {
        Q_DISABLE_COPY(ObfRoutingSectionInfo_P)
    private:
    protected:
        ObfRoutingSectionInfo_P(ObfRoutingSectionInfo* owner);

        ImplementationInterface<ObfRoutingSectionInfo> owner;

        struct EncodingRule
        {
            enum Type : uint32_t
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

            uint32_t _id;
            QString _tag;
            QString _value;
            Type _type;
            union
            {
                int32_t asSignedInt;
                uint32_t asUnsignedInt;
                float asFloat;
            } _parsedValue;

            bool isRoundabout() const;
            Model::RoadDirection getDirection() const;
        };
        QList< std::shared_ptr<EncodingRule> > _encodingRules;

        uint32_t _borderBoxOffset;
        uint32_t _baseBorderBoxOffset;
        uint32_t _borderBoxLength;
        uint32_t _baseBorderBoxLength;
    public:
        virtual ~ObfRoutingSectionInfo_P();

    friend class OsmAnd::ObfRoutingSectionInfo;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::Model::Road;
    friend class OsmAnd::RoutePlanner;
    friend class OsmAnd::RoutePlannerContext;
    friend class OsmAnd::RoutingRulesetContext;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_INFO_P_H_)
