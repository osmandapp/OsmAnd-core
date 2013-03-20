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

#ifndef __ROUTING_CONFIGURATION_H_
#define __ROUTING_CONFIGURATION_H_

#include <cstdint>

#include <QIODevice>
#include <QString>
#include <QMap>
#include <QHash>
#include <QStack>
#include <QXmlStreamReader>

#include <OsmAndCore.h>
#include <ObfRoutingSection.h>
#include <RoutingProfile.h>
#include <RoutingRulesetContext.h>

namespace OsmAnd {

    class OSMAND_CORE_API RoutingConfiguration
    {
    private:
        static const uint8_t _defaultRawXml[];
        static const size_t _defaultRawXmlSize;

        static void parseRoutingProfile(QXmlStreamReader* xmlParser, std::shared_ptr<RoutingProfile> routingProfile);
        static void parseRoutingParameter(QXmlStreamReader* xmlParser, std::shared_ptr<RoutingProfile> routingProfile);
        static void parseRoutingRuleset(QXmlStreamReader* xmlParser, std::shared_ptr<RoutingProfile> routingProfile, RoutingProfile::RulesetType rulesetType, QStack< std::shared_ptr<struct RoutingRule> >& ruleset);
        static void addRulesetSubclause(std::shared_ptr<struct RoutingRule> routingRule, std::shared_ptr<RoutingRulesetContext> context);
        static bool isConditionTag(const QStringRef& tagName);
    protected:
        QHash< QString, QString > _attributes;
    public:
        RoutingConfiguration();
        virtual ~RoutingConfiguration();

        // 1. parameters of routing and different tweaks
        // Influence on A* : f(x) + heuristicCoefficient*g(X)
        //public float heuristicCoefficient = 1;

        // 1.1 tile load parameters (should not affect routing)
        //public int ZOOM_TO_LOAD_TILES = 16;
        //public int memoryLimitation;

        // 1.2 Build A* graph in backward/forward direction (can affect results)
        // 0 - 2 ways, 1 - direct way, -1 - reverse way
        //public int planRoadDirection = 0;

        // 1.3 Router specific coefficients and restrictions
        QMap< QString, std::shared_ptr<RoutingProfile> > _routingProfiles;
        std::shared_ptr<RoutingProfile> _defaultRoutingProfile;
        QString _defaultRoutingProfileName;

        // 1.4 Used to calculate route in movement
        //public Double initialDirection;

        // 1.5 Recalculate distance help
        //public float recalculateDistance = 10000f;

        static bool parseConfiguration(QIODevice* data, RoutingConfiguration& outConfig);
        static void loadDefault(RoutingConfiguration& outConfig);
    };

} // namespace OsmAnd

#endif // __ROUTING_CONFIGURATION_H_
