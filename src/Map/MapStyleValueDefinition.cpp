#include "MapStyleValueDefinition.h"

#include <cassert>

QAtomicInt OsmAnd::MapStyleValueDefinition::_nextId(1);

OsmAnd::MapStyleValueDefinition::MapStyleValueDefinition( const MapStyleValueClass valueClass_, const MapStyleValueDataType dataType_, const QString& name_, const bool isComplex_ )
    : id(_nextId.fetchAndAddOrdered(1))
    , valueClass(valueClass_)
    , dataType(dataType_)
    , name(name_)
    , isComplex(isComplex_)
{
    // This odd code checks if _nextRuntimeGeneratedId was initialized prior to call of this ctor.
    // This may happen if this ctor is called by initialization of other static variable. And
    // since their initialization order is not defined, this may lead to very odd bugs.
    assert(_nextId.load() >= 2);
}

OsmAnd::MapStyleValueDefinition::~MapStyleValueDefinition()
{
}
