#ifndef _OSMAND_CORE_ROUTING_RULESET_H_
#define _OSMAND_CORE_ROUTING_RULESET_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/Routing/RoutingRuleExpression.h>

namespace OsmAnd {

    class RoutingProfile;

    class OSMAND_CORE_API RoutingRuleset
    {
    public:
        WEAK_ENUM_EX(Type, int)
        {
            Invalid = -1,
            RoadPriorities = 0,
            RoadSpeed,
            Access,
            Obstacles,
            RoutingObstacles,
            OneWay,

            TypesCount
        };
    private:
        QList< std::shared_ptr<RoutingRuleExpression> > _expressions;
    protected:
        void registerSelectExpression(const QString& value, const QString& type);
        RoutingRuleset(RoutingProfile* owner, const Type type);
    public:
        virtual ~RoutingRuleset();

        RoutingProfile* const owner;
        const Type type;
        const QList< std::shared_ptr<RoutingRuleExpression> >& expressions;

        friend class OsmAnd::RoutingProfile;
        friend class OsmAnd::RoutingRulesetContext;
        friend class OsmAnd::RoutingConfiguration;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTING_RULESET_H_)
