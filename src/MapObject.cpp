#include "MapObject.h"

OsmAnd::Model::MapObject::MapObject(ObfMapSection* section_)
    : section(section_)
    , id(_id)
    , names(_names)
{
}

OsmAnd::Model::MapObject::~MapObject()
{
}
