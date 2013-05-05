#include <Street.h>

OsmAnd::Model::Street::Street()
    : id(_id)
    , name(_name)
    , latinName(_name)
    , xTile24(_xTile24)
    , yTile24(_yTile24)
    , longitude(_longitude)
    , latitude(_latitude)
{
}

OsmAnd::Model::Street::~Street()
{
}
