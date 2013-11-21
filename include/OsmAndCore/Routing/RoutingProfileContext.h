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

#ifndef _OSMAND_CORE_ROUTING_PROFILE_CONTEXT_H_
#define _OSMAND_CORE_ROUTING_PROFILE_CONTEXT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/Routing/RoutingProfile.h>
#include <OsmAndCore/Routing/RoutingRulesetContext.h>
#include <OsmAndCore/Data/Model/Road.h>

namespace OsmAnd {

    class ObfRoutingSectionInfo;
    class ObfRoutingBorderLinePoint;

    class OSMAND_CORE_API RoutingProfileContext
    {
    private:
    protected:
        std::shared_ptr<RoutingRulesetContext> _rulesetContexts[RoutingRuleset::TypesCount];

        QHash< std::shared_ptr<const ObfRoutingSectionInfo>, QMap<uint32_t, uint32_t> > _tagValueAttribIdCache;
    public:
        RoutingProfileContext(const std::shared_ptr<RoutingProfile>& profile, QHash<QString, QString>* contextValues = nullptr);
        virtual ~RoutingProfileContext();

        const std::shared_ptr<RoutingProfile> profile;

        std::shared_ptr<RoutingRulesetContext> getRulesetContext(RoutingRuleset::Type type);

        Model::RoadDirection getDirection(const std::shared_ptr<const OsmAnd::Model::Road>& road);
        bool acceptsRoad(const std::shared_ptr<const OsmAnd::Model::Road>& road);
        bool acceptsBorderLinePoint(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const std::shared_ptr<const ObfRoutingBorderLinePoint>& point);
        float getSpeedPriority(const std::shared_ptr<const OsmAnd::Model::Road>& road);
        float getSpeed(const std::shared_ptr<const OsmAnd::Model::Road>& road);
        float getObstaclesExtraTime(const std::shared_ptr<const OsmAnd::Model::Road>& road, uint32_t pointIndex);
        float getRoutingObstaclesExtraTime(const std::shared_ptr<const OsmAnd::Model::Road>& road, uint32_t pointIndex);

        friend class OsmAnd::RoutingRulesetContext;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_ROUTING_PROFILE_CONTEXT_H_