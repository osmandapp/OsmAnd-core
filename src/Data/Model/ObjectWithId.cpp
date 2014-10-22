#include "ObjectWithId.h"

OsmAnd::Model::ObjectWithId::ObjectWithId()
    : _id(ObfObjectId::invalidId())
    , id(_id)
{
}

OsmAnd::Model::ObjectWithId::~ObjectWithId()
{
}
