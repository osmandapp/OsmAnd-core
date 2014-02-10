#ifndef _OSMAND_CORE_ROUTING_RULE_EXPRESSION_H_
#define _OSMAND_CORE_ROUTING_RULE_EXPRESSION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QSet>
#include <QHash>
#include <QBitArray>

#include <OsmAndCore.h>

namespace OsmAnd {

    class RoutingConfiguration;
    class RoutingRuleset;
    class RoutingRulesetContext;

    class OSMAND_CORE_API RoutingRuleExpression
    {
    public:
        class OSMAND_CORE_API Operator
        {
        private:
        protected:
            Operator();
        public:
            virtual ~Operator();

            virtual bool evaluate(const QBitArray& types, RoutingRulesetContext* const context) const = 0;

            friend class OsmAnd::RoutingRuleExpression;
        };
    private:
        QList<QString> _parameters;

        QString _variableRef;
        QString _tagRef;
        float _value;

        QBitArray _filterTypes;
        QBitArray _filterNotTypes;
        QSet<QString> _onlyTags;
        QSet<QString> _onlyNotTags;

        QList< std::shared_ptr<Operator> > _operators;

        bool validateAllTypesShouldBePresent(const QBitArray& types) const;
        bool validateAllTypesShouldNotBePresent(const QBitArray& types) const;
        bool validateFreeTags(const QBitArray& types) const;
        bool validateNotFreeTags(const QBitArray& types) const;
        bool evaluateExpressions(const QBitArray& types, RoutingRulesetContext* const context) const;
    protected:
        void registerAndTagValue(const QString& tag, const QString& value, const bool negation);
        void registerLessCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerLessOrEqualCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerGreaterCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerGreaterOrEqualCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerAndParamCondition(const QString& param, const bool negation);
        
        RoutingRuleExpression(RoutingRuleset* const ruleset, const QString& value, const QString& type);

        QString _type;
    public:
        virtual ~RoutingRuleExpression();

        RoutingRuleset* const ruleset;
        const QString& type;
        const QList<QString>& parameters;

        enum ResultType
        {
            Integer,
            Float
        };
        bool validate(const QBitArray& types, RoutingRulesetContext* context) const;
        bool evaluate(const QBitArray& types, RoutingRulesetContext* context, ResultType type, void* result) const;

        static bool resolveVariableReferenceValue(RoutingRulesetContext* context, const QString& variableRef, const QString& type, float& value);
        static bool resolveTagReferenceValue(RoutingRulesetContext* context, const QBitArray& types, const QString& tagRef, const QString& type, float& value);

        friend class OsmAnd::RoutingConfiguration;
        friend class OsmAnd::RoutingRuleset;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTING_RULE_EXPRESSION_H_)
