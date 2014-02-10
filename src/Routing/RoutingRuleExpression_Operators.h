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

#endif // !defined(_OSMAND_CORE_ROUTING_RULE_EXPRESSION_OPERATORS_H_)