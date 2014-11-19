#include "MapStyleValueDefinition.h"

#include "Logging.h"

OsmAnd::MapStyleValueDefinition::MapStyleValueDefinition(
    const Class valueClass_,
    const MapStyleValueDataType dataType_,
    const QString& name_,
    const bool isComplex_)
    : valueClass(valueClass_)
    , dataType(dataType_)
    , name(name_)
    , isComplex(isComplex_)
{
    if (isComplex && (dataType != MapStyleValueDataType::Float && dataType != MapStyleValueDataType::Integer))
    {
        LogPrintf(LogSeverityLevel::Error,
            "'%s' map style value definition is declared as complex, but its data type is not float nor integer",
            qPrintable(name));
    }
}

OsmAnd::MapStyleValueDefinition::~MapStyleValueDefinition()
{
}
