/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

#endif // _OSMAND_CORE_ROUTING_CONFIGURATION_H_
