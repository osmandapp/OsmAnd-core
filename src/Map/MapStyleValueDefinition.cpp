#include "MapStyleValueDefinition.h"

OsmAnd::MapStyleValueDefinition::MapStyleValueDefinition( const MapStyleValueClass& valueClass_, const MapStyleValueDataType& dataType_, const QString& name_, const bool& isComplex_ )
    : valueClass(valueClass_)
    , dataType(dataType_)
    , name(name_)
    , isComplex(isComplex_)
{
}

OsmAnd::MapStyleValueDefinition::~MapStyleValueDefinition()
{
}
