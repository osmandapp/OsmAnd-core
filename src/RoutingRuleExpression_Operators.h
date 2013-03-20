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

#ifndef __ROUTING_RULE_EXPRESSION_OPERATORS_H_
#define __ROUTING_RULE_EXPRESSION_OPERATORS_H_

#include <QString>

namespace OsmAnd {

    class RoutingRuleExpressionOperator
    {
    private:
    protected:
    public:
        RoutingRuleExpressionOperator();
        virtual ~RoutingRuleExpressionOperator();
    };

    class RoutingRuleExpressionBinaryOperator : public RoutingRuleExpressionOperator
    {
    private:
    protected:
    public:
        RoutingRuleExpressionBinaryOperator(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~RoutingRuleExpressionBinaryOperator();
    };

    class Operator_G : public RoutingRuleExpressionBinaryOperator
    {
    private:
    protected:
    public:
        Operator_G(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_G();
    };

    class Operator_GE : public RoutingRuleExpressionBinaryOperator
    {
    private:
    protected:
    public:
        Operator_GE(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_GE();
    };

    class Operator_L : public RoutingRuleExpressionBinaryOperator
    {
    private:
    protected:
    public:
        Operator_L(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_L();
    };

    class Operator_LE : public RoutingRuleExpressionBinaryOperator
    {
    private:
    protected:
    public:
        Operator_LE(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_LE();
    };

} // namespace OsmAnd

#endif // __ROUTING_RULE_EXPRESSION_OPERATORS_H_