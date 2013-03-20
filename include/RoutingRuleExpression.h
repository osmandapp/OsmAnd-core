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

#ifndef __ROUTING_RULE_EXPRESSION_H_
#define __ROUTING_RULE_EXPRESSION_H_

#include <cstdint>
#include <memory>

#include <QString>
#include <QSet>
#include <QHash>

#include <OsmAndCore.h>

namespace OsmAnd {

    class OSMAND_CORE_API RoutingRuleExpression
    {
    private:
        QList<QString> _parameters;
/*
            Object selectValue;
        QString selectType;
        QBitSet filterTypes;
        QBitSet filterNotTypes;
        QBitSet evalFilterTypes;*/
        QSet<QString> _onlyTags;
        QSet<QString> _onlyNotTags;
        QList< std::shared_ptr<class RoutingRuleExpressionOperator> > _operators;
    protected:
        void registerAndTagValue(const QString& tag, const QString& value, bool negation);
        void registerLessCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerLessOrEqualCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerGreaterCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerGreaterOrEqualCondition(const QString& lvalue, const QString& rvalue, const QString& type);
        void registerAndParamCondition(const QString& param, bool negation);
    public:
        RoutingRuleExpression(const QString& value, const QString& type);
        virtual ~RoutingRuleExpression();

    friend class RoutingConfiguration;
        

        
    /*
        public Object eval(BitSet types, ParameterContext paramContext) {
            if (matches(types, paramContext)) {
                return calcSelectValue(types, paramContext);
            }
            return null;
        }

        protected Object calcSelectValue(BitSet types, ParameterContext paramContext) {
            if (selectValue instanceof String && selectValue.toString().startsWith("$")) {
                BitSet mask = tagRuleMask.get(selectValue.toString().substring(1));
                if (mask != null && mask.intersects(types)) {
                    BitSet findBit = new BitSet(mask.size());
                    findBit.or(mask);
                    findBit.and(types);
                    int value = findBit.nextSetBit(0);
                    return parseValueFromTag(value, selectType);
                }
            } else if (selectValue instanceof String && selectValue.toString().startsWith(":")) {
                String p = ((String) selectValue).substring(1);
                if (paramContext != null && paramContext.vars.containsKey(p)) {
                    selectValue = parseValue(paramContext.vars.get(p), selectType);
                } else {
                    return null;
                }
            }
            return selectValue;
        }

        public boolean matches(BitSet types, ParameterContext paramContext) {
            if(!checkAllTypesShouldBePresent(types)) {
                return false;
            }
            if(!checkAllTypesShouldNotBePresent(types)) {
                return false;
            }
            if(!checkFreeTags(types)) {
                return false;
            }
            if(!checkNotFreeTags(types)) {
                return false;
            }
            if(!checkExpressions(types, paramContext)) {
                return false;
            }
            return true;
        }

        private boolean checkExpressions(BitSet types, ParameterContext paramContext) {
            for(RouteAttributeExpression e : expressions) {
                if(!e.matches(types, paramContext)) {
                    return false;
                }
            }
            return true;
        }

        private boolean checkFreeTags(BitSet types) {
            for (String ts : onlyTags) {
                BitSet b = tagRuleMask.get(ts);
                if (b == null || !b.intersects(types)) {
                    return false;
                }
            }
            return true;
        }

        private boolean checkNotFreeTags(BitSet types) {
            for (String ts : onlyNotTags) {
                BitSet b = tagRuleMask.get(ts);
                if (b != null && b.intersects(types)) {
                    return false;
                }
            }
            return true;
        }

        private boolean checkAllTypesShouldNotBePresent(BitSet types) {
            if(filterNotTypes.intersects(types)) {
                return false;
            }
            return true;
        }

        private boolean checkAllTypesShouldBePresent(BitSet types) {
            // Bitset method subset is missing "filterTypes.isSubset(types)"
            // reset previous evaluation
            evalFilterTypes.or(filterTypes);
            // evaluate bit intersection and check if filterTypes contained as set in types
            evalFilterTypes.and(types);
            if(!evalFilterTypes.equals(filterTypes)) {
                return false;
            }
            return true;
        }
        */
    };

} // namespace OsmAnd

#endif // __ROUTING_RULE_EXPRESSION_H_