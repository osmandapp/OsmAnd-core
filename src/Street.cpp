#include <Street.h>

OsmAnd::Model::Street::Street()
    : id(_id)
    , name(_name)
    , latinName(_name)
    , tile24(_tile24)
    , location(_location)
{
}

OsmAnd::Model::Street::~Street()
{
}
