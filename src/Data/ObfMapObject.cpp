#include "ObfMapObject.h"

OsmAnd::ObfMapObject::ObfMapObject(const std::shared_ptr<const ObfSectionInfo>& obfSection_)
    : id(ObfObjectId::invalidId())
    , obfSection(obfSection_)
{
}

OsmAnd::ObfMapObject::~ObfMapObject()
{
}

QString OsmAnd::ObfMapObject::toString() const
{
    return id.toString();
}
