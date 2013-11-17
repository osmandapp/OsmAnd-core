#include "MapStyleConfigurableInputValue.h"

OsmAnd::MapStyleConfigurableInputValue::MapStyleConfigurableInputValue( const MapStyleValueDataType dataType_, const QString& name_, const QString& title_, const QString& description_, const QStringList& possibleValues_ )
    : MapStyleValueDefinition(MapStyleValueClass::Input, dataType_, name_, false)
    , title(title_)
    , description(description_)
    , possibleValues(possibleValues_)
{
}

OsmAnd::MapStyleConfigurableInputValue::~MapStyleConfigurableInputValue()
{
}
