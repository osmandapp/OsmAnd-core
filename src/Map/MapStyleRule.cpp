#include "MapStyleRule.h"
#include "MapStyleRule_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::MapStyleNode::MapStyleRule(MapStyle* const style_, const QHash< QString, QString >& attributes)
    : _p(new MapStyleRule_P(this))
    , style(style_)
{
    _p->_values.reserve(attributes.size());
    _p->_resolvedValueDefinitions.reserve(attributes.size());

    for (const auto& attributeEntry : rangeOf(constOf(attributes)))
    {
        const auto& key = attributeEntry.key();
        const auto& value = attributeEntry.value();

        std::shared_ptr<const MapStyleValueDefinition> valueDef;
        bool ok = owner->resolveValueDefinition(key, valueDef);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Style '%s' references unsupported attribute '%s', skipping it",
                qPrintable(owner->name),
                qPrintable(key));
            continue;
        }

        // Parse value
        MapStyleValue parsedValue;
        ok = owner_->_p->parseValue(valueDef, value, parsedValue, true);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Style '%s' attribute '%s' has unsupported value '%s', skipping it",
                qPrintable(owner->name),
                qPrintable(key),
                qPrintable(value));
            continue;
        }

        // Store resolved value definition
        _p->_resolvedValueDefinitions.insert(key, valueDef);

        // And store value itself
        _p->_values.insert(valueDef, parsedValue);
    }
}

OsmAnd::MapStyleNode::~MapStyleRule()
{
}

bool OsmAnd::MapStyleNode::getAttribute(const std::shared_ptr<const MapStyleValueDefinition>& key, MapStyleValue& value) const
{
    auto itValue = _p->_values.constFind(key);
    if (itValue == _p->_values.cend())
        return false;

    value = *itValue;
    return true;
}

void OsmAnd::MapStyleNode::dump(const QString& prefix /*= QString::null*/) const
{
    auto newPrefix = prefix + QLatin1String("\t");

    for (const auto& valueEntry : rangeOf(constOf(_p->_values)))
    {
        const auto& valueDef = valueEntry.key();
        const auto& value = valueEntry.value();

        QString strValue;
        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Boolean:
            strValue = (value.asSimple.asInt == 1) ? QLatin1String("true") : QLatin1String("false");
            break;
        case MapStyleValueDataType::Integer:
            if (value.isComplex)
                strValue = QString::fromLatin1("%1:%2").arg(value.asComplex.asInt.dip).arg(value.asComplex.asInt.px);
            else
                strValue = QString::fromLatin1("%1").arg(value.asSimple.asInt);
            break;
        case MapStyleValueDataType::Float:
            if (value.isComplex)
                strValue = QString::fromLatin1("%1:%2").arg(value.asComplex.asFloat.dip).arg(value.asComplex.asFloat.px);
            else
                strValue = QString::fromLatin1("%1").arg(value.asSimple.asFloat);
            break;
        case MapStyleValueDataType::String:
            strValue = owner->_p->lookupStringValue(value.asSimple.asUInt);
            break;
        case MapStyleValueDataType::Color:
        {
            auto color = value.asSimple.asUInt;
            if ((color & 0xFF000000) == 0xFF000000)
                strValue = '#' + QString::number(color, 16).right(6);
            else
                strValue = '#' + QString::number(color, 16).right(8);
        }
            break;
        }

        LogPrintf(LogSeverityLevel::Debug, "%s%s%s = %s",
            qPrintable(newPrefix),
            (valueDef->valueClass == MapStyleValueClass::Input) ? ">" : "<",
            qPrintable(valueDef->name),
            qPrintable(strValue));
    }

    if (!_p->_ifChildren.empty())
    {
        LogPrintf(LogSeverityLevel::Debug, "%sIf(",
            qPrintable(newPrefix));
        for (const auto& child : constOf(_p->_ifChildren))
        {
            LogPrintf(LogSeverityLevel::Debug, "%sAND %p",
                qPrintable(newPrefix),
                child.get());
            child->dump(newPrefix);
        }
        LogPrintf(LogSeverityLevel::Debug, "%s)",
            qPrintable(newPrefix));
    }

    if (!_p->_ifElseChildren.empty())
    {
        LogPrintf(LogSeverityLevel::Debug, "%sSelector: [",
            qPrintable(newPrefix));
        for (const auto& child : constOf(_p->_ifElseChildren))
        {
            LogPrintf(LogSeverityLevel::Debug, "%sOR %p",
                qPrintable(newPrefix),
                child.get());
            child->dump(newPrefix);
        }
        LogPrintf(LogSeverityLevel::Debug, "%s]",
            qPrintable(newPrefix));
    }
}
