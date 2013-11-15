#include "MapStyleValueDefinition.h"

QAtomicInt OsmAnd::MapStyleValueDefinition::_nextId(1);

OsmAnd::MapStyleValueDefinition::MapStyleValueDefinition( const MapStyleValueClass valueClass_, const MapStyleValueDataType dataType_, const QString& name_, const bool isComplex_ )
    : id(_nextId.fetchAndAddOrdered(1))
    , valueClass(valueClass_)
    , dataType(dataType_)
    , name(name_)
    , isComplex(isComplex_)
{
}

OsmAnd::MapStyleValueDefinition::~MapStyleValueDefinition()
{
}
