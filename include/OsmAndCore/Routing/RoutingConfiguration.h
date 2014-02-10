#ifndef _OSMAND_CORE_ROUTING_CONFIGURATION_H_
#define _OSMAND_CORE_ROUTING_CONFIGURATION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QIODevice>
#include <QString>
#include <QMap>
#include <QHash>
#include <QStack>
#include <QXmlStreamReader>

#include <OsmAndCore.h>
#include <OsmAndCore/Routing/RoutingProfile.h>
#include <OsmAndCore/Routing/RoutingRulesetContext.h>

namespace OsmAnd {

    class OSMAND_CORE_API RoutingConfiguration
    {
    private:
        static void parseRoutingProfile(QXmlStreamReader* xmlParser, RoutingProfile* routingProfile);
        static void parseRoutingParameter(QXmlStreamReader* xmlParser, RoutingProfile* routingProfile);
        static void parseRoutingRuleset(QXmlStreamReader* xmlParser, RoutingProfile* routingProfile, RoutingRuleset::Type rulesetType, QStack< std::shared_ptr<struct RoutingRule> >& ruleset);
        static void addRulesetSubclause(struct RoutingRule* routingRule, RoutingRuleset* ruleset);
        static bool isConditionTag(const QStringRef& tagName);
    protected:
        QHash< QString, QString > _attributes;

        QMap< QString, std::shared_ptr<RoutingProfile> > _routingProfiles;
        std::shared_ptr<RoutingProfile> _defaultRoutingProfile;
        QString _defaultRoutingProfileName;
    public:
        RoutingConfiguration();
        virtual ~RoutingConfiguration();

        const QMap< QString, std::shared_ptr<OsmAnd::RoutingProfile> >& routingProfiles;

        QString resolveAttribute(const QString& vehicle, const QString& name);

        static bool parseTypedValue(const QString& value, const QString& type, float& parsedValue);

        static bool parseConfiguration(QIODevice* data, OsmAnd::RoutingConfiguration& outConfig);
        static void loadDefault(OsmAnd::RoutingConfiguration& outConfig);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTING_CONFIGURATION_H_)
