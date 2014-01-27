/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#endif // _OSMAND_CORE_ROUTING_RULE_EXPRESSION_H_
