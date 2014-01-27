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

#ifndef _OSMAND_CORE_ROUTING_RULE_EXPRESSION_OPERATORS_H_
#define _OSMAND_CORE_ROUTING_RULE_EXPRESSION_OPERATORS_H_

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include "RoutingRuleExpression.h"

namespace OsmAnd {

    class BinaryOperator : public RoutingRuleExpression::Operator
    {
    private:
    protected:
        QString _lVariableRef;
        QString _lTagRef;
        float _lValue;
        
        QString _rVariableRef;
        QString _rTagRef;
        float _rValue;
        
        BinaryOperator(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual bool evaluateValues(float lValue, float rValue) const = 0;
    public:
        virtual ~BinaryOperator();

        const QString type;

        virtual bool evaluate(const QBitArray& types, RoutingRulesetContext* const context) const;
    };

    class Operator_G : public BinaryOperator
    {
    private:
    protected:
        virtual bool evaluateValues(float lValue, float rValue) const;
    public:
        Operator_G(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_G();
    };

    class Operator_GE : public BinaryOperator
    {
    private:
    protected:
        virtual bool evaluateValues(float lValue, float rValue) const;
    public:
        Operator_GE(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_GE();
    };

    class Operator_L : public BinaryOperator
    {
    private:
    protected:
        virtual bool evaluateValues(float lValue, float rValue) const;
    public:
        Operator_L(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_L();
    };

    class Operator_LE : public BinaryOperator
    {
    private:
    protected:
        virtual bool evaluateValues(float lValue, float rValue) const;
    public:
        Operator_LE(const QString& lvalue, const QString& rvalue, const QString& type);
        virtual ~Operator_LE();
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_ROUTING_RULE_EXPRESSION_OPERATORS_H_