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

#ifndef __ROUTING_PROFILE_H_
#define __ROUTING_PROFILE_H_

#include <cstdint>
#include <memory>

#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <RoutingRulesetContext.h>

namespace OsmAnd {

    class OSMAND_CORE_API RoutingProfile
    {
    public:
        enum RulesetType : int {
            Invalid = -1,
            RoadPriorities = 0,
            RoadSpeed,
            Access,
            Obstacles,
            RoutingObstacles,
            OneWay,

            RulesetTypesCount
        };
    private:
        struct Parameter
        {
            enum Type
            {
                Numeric,
                Boolean,
            };

            QString _id;
            QString _name;
            QString _description;
            Type _type;
            QList<double> _possibleValues;
            QList<QString> _possibleValueDescriptions;
        };
        QHash< QString, std::shared_ptr<Parameter> > _parameters;
        std::shared_ptr<RoutingRulesetContext> _rulesetContexts[RulesetTypesCount];
    protected:
        void registerBooleanParameter(const QString& id, const QString& name, const QString& description);
        void registerNumericParameter(const QString& id, const QString& name, const QString& description, QList<double>& values, QList<QString> valuesDescriptions);
    public:
        RoutingProfile();
        virtual ~RoutingProfile();
        /*
        enum Preset {
            Car,
            Pedestrian,
            Bicycle
        };
        */

        QString _name;

        // Cached values
        bool _restrictionsAware;
        float _leftTurn;
        float _roundaboutTurn;
        float _rightTurn;
        float _minDefaultSpeed;
        float _maxDefaultSpeed;

        QHash<QString, QString> _attributes;
        void addAttribute(const QString& key, const QString& value);

        std::shared_ptr<RoutingRulesetContext> getRulesetContext(RulesetType type);

    friend class RoutingConfiguration;
    };

} // namespace OsmAnd

#endif // __ROUTING_PROFILE_H_