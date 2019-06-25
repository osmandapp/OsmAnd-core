#ifndef _OSMAND_CORE_ROUTING_PROFILE_H_
#define _OSMAND_CORE_ROUTING_PROFILE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
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
        float _minSpeed;
        float _defaultSpeed;
        float _maxSpeed;
    protected:
        QHash< QString, std::shared_ptr<Parameter> > _parameters;

        void registerBooleanParameter(const QString& id, const QString& name, const QString& description);
        void registerNumericParameter(const QString& id, const QString& name, const QString& description, QList<double>& values, const QStringList& valuesDescriptions);
        uint32_t registerTagValueAttribute(const QString& tag, const QString& value);
        bool parseTypedValueFromTag(const uint32_t id, const QString& type, float& parsedValue);
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
        const float& minSpeed;
        const float& defaultSpeed;
        const float& maxSpeed;

        std::shared_ptr<OsmAnd::RoutingRuleset> getRuleset(const OsmAnd::RoutingRuleset::Type type) const;
        void addAttribute(const QString& key, const QString& value);

    friend class OsmAnd::RoutingConfiguration;
    friend class OsmAnd::RoutingRuleExpression;
    friend class OsmAnd::RoutingRulesetContext;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTING_PROFILE_H_)
