#include "ObfSection.h"

OsmAnd::ObfSection::ObfSection( ObfReader* owner_ )
    : name(_name)
    , length(_length)
    , offset(_offset)
    , owner(owner_)
{
}

OsmAnd::ObfSection::~ObfSection()
{
}
