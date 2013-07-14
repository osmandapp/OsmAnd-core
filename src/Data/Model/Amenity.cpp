#include "Amenity.h"

OsmAnd::Model::Amenity::Amenity()
    : id(_id)
    , name(_name)
    , latinName(_latinName)
    , point31(_point31)
    , openingHours(_openingHours)
    , site(_site)
    , phone(_phone)
    , description(_description)
    , categoryId(_categoryId)
    , subcategoryId(_subcategoryId)
{
}

OsmAnd::Model::Amenity::~Amenity()
{
}

OsmAnd::Model::Amenity::Category::Category()
    : name(_name)
    , subcategories(_subcategories)
{
}

OsmAnd::Model::Amenity::Category::~Category()
{
}
