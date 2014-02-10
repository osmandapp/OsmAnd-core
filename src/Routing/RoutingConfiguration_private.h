#ifndef _OSMAND_CORE_ROUTING_CONFIGURATION_PRIVATE_H_
#define _OSMAND_CORE_ROUTING_CONFIGURATION_PRIVATE_H_

namespace OsmAnd {

    struct RoutingRule
    {
        QString _tagName;
        QString _t;
        QString _v;
        QString _param;
        QString _value1;
        QString _value2;
        QString _type;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTING_CONFIGURATION_PRIVATE_H_)
