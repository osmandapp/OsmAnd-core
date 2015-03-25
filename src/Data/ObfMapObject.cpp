#include "ObfMapObject.h"

#include "ObfSectionInfo.h"

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
    return id.toString() + QLatin1String(" from ") + obfSection->name;
}
