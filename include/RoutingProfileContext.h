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

#ifndef __ROUTING_PROFILE_CONTEXT_H_
#define __ROUTING_PROFILE_CONTEXT_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <ObfRoutingSection.h>
#include <RoutingProfile.h>
#include <RoutingRulesetContext.h>
#include <Road.h>

namespace OsmAnd {

    class OSMAND_CORE_API RoutingProfileContext
    {
    private:
    protected:
        std::shared_ptr<RoutingRulesetContext> _rulesetContexts[RoutingRuleset::TypesCount];

        QMap< ObfRoutingSection*, QMap<uint32_t, uint32_t> > _tagValueAttribIdCache;
    public:
        RoutingProfileContext(std::shared_ptr<RoutingProfile> profile, QHash<QString, QString>* contextValues = nullptr);
        virtual ~RoutingProfileContext();

        const std::shared_ptr<RoutingProfile> profile;

        std::shared_ptr<RoutingRulesetContext> getRulesetContext(RoutingRuleset::Type type);

        Model::Road::Direction getDirection(Model::Road* road);
        bool acceptsRoad(Model::Road* road);
        bool acceptsBorderLinePoint(ObfRoutingSection* section, ObfRoutingSection::BorderLinePoint* point);
        float getSpeedPriority(Model::Road* road);
        float getSpeed(Model::Road* road);
        float getObstaclesExtraTime(Model::Road* road, uint32_t pointIndex);
        float getRoutingObstaclesExtraTime(Model::Road* road, uint32_t pointIndex);

        friend class RoutingRulesetContext;
    };

} // namespace OsmAnd

#endif // __ROUTING_PROFILE_CONTEXT_H_