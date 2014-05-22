#include "ObjectWithId.h"

OsmAnd::Model::ObjectWithId::ObjectWithId()
    : _id(std::numeric_limits<uint64_t>::max())
    , id(_id)
{
}

OsmAnd::Model::ObjectWithId::~ObjectWithId()
{
}
