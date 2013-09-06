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
#include <QMap>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/Routing/RoutingRuleset.h>
#include <OsmAndCore/Data/Model/Road.h>

namespace OsmAnd {

    class OSMAND_CORE_API RoutingProfile
    {
    public:
        class OSMAND_CORE_API Parameter
        {
        public:
            enum Type
            {
                Numeric,
                Boolean,
            };
        private:
        protected:
            QString _id;
            QString _name;
            QString _description;
            Type _type;
            QList<double> _possibleValues;
            QList<QString> _possibleValueDescriptions;

            Parameter();
        public:
            virtual ~Parameter();

            const QString& id;
            const QString& name;
            const QString& description;
            const Type& type;
            const QList<double>& possibleValues;
            const QList<QString>& possibleValueDescriptions;

        friend class OsmAnd::RoutingProfile;
        };
    private:
        QString _name;
        QHash<QString, QString> _attributes;
        std::shared_ptr<RoutingRuleset> _rulesets[RoutingRuleset::TypesCount];

        QHash<QString, uint32_t> _universalRules;
        QList<QString> _universalRulesKeysById;
        QHash<QString, QBitArray> _tagRuleMask;
        QMap<uint32_t, float> _ruleToValueCache;
        
        // Cached values
        bool _restrictionsAware;
        bool _oneWayAware;
        bool _followSpeedLimitations;
        float _leftTurn;
        float _roundaboutTurn;
        float _rightTurn;
        float _minDefaultSpeed;
        float _maxDefaultSpeed;
    protected:
        QHash< QString, std::shared_ptr<Parameter> > _parameters;

        void registerBooleanParameter(const QString& id, const QString& name, const QString& description);
        void registerNumericParameter(const QString& id, const QString& name, const QString& description, QList<double>& values, const QStringList& valuesDescriptions);
        uint32_t registerTagValueAttribute(const QString& tag, const QString& value);
        bool parseTypedValueFromTag(uint32_t id, const QString& type, float& parsedValue);
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

        const QString& name;
        const QHash<QString, QString>& attributes;
        const QHash< QString, std::shared_ptr<OsmAnd::RoutingProfile::Parameter> >& parameters;

        const bool& restrictionsAware;
        const float& leftTurn;
        const float& roundaboutTurn;
        const float& rightTurn;
        const float& minDefaultSpeed;
        const float& maxDefaultSpeed;

        std::shared_ptr<OsmAnd::RoutingRuleset> getRuleset(OsmAnd::RoutingRuleset::Type type) const;
        void addAttribute(const QString& key, const QString& value);

    friend class OsmAnd::RoutingConfiguration;
    friend class OsmAnd::RoutingRuleExpression;
    friend class OsmAnd::RoutingRulesetContext;
    };

} // namespace OsmAnd

#endif // __ROUTING_PROFILE_H_