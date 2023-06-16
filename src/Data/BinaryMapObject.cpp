#include "BinaryMapObject.h"

#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"

OsmAnd::BinaryMapObject::BinaryMapObject(
    const std::shared_ptr<const ObfMapSectionInfo>& section_,
    const std::shared_ptr<const ObfMapSectionLevel>& level_)
    : ObfMapObject(section_)
    , section(section_)
    , level(level_)
    , blockId(ObfMapSectionDataBlockId::invalidId())
{
    attributeMapping = section->getAttributeMapping();
}

OsmAnd::BinaryMapObject::~BinaryMapObject()
{
}

QString OsmAnd::BinaryMapObject::toString() const
{
    return ObfMapObject::toString() + QString(QLatin1String("@[%1-%2]")).arg(level->minZoom).arg(level->maxZoom);
}

bool OsmAnd::BinaryMapObject::obtainSharingKey(SharingKey& outKey) const
{
    outKey = id;
    return true;
}

bool OsmAnd::BinaryMapObject::obtainSortingKey(SortingKey& outKey) const
{
    outKey = id;
    return true;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObject::getMinZoomLevel() const
{
    return level->minZoom;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObject::getMaxZoomLevel() const
{
    return level->maxZoom;
}

OsmAnd::MapObject::LayerType OsmAnd::BinaryMapObject::getLayerType() const
{
    const auto& obfAttributeMapping = std::static_pointer_cast<const ObfMapSectionAttributeMapping>(attributeMapping);

    for (const auto& additionalAttributeId : constOf(additionalAttributeIds))
    {
        if (obfAttributeMapping->positiveLayerAttributeIds.contains(additionalAttributeId))
            return LayerType::Positive;
        else if (obfAttributeMapping->negativeLayerAttributeIds.contains(additionalAttributeId))
            return LayerType::Negative;
        else if (obfAttributeMapping->zeroLayerAttributeIds.contains(additionalAttributeId))
            return LayerType::Zero;
    }

    return LayerType::Zero;
}

QString OsmAnd::BinaryMapObject::debugInfo(long i, bool all) const
{
    QString s = "";
    if (all || i == id.id)
    {
        s = id.toString();
        for (const auto& caption : captions.values())
        {
            s.append(" \"" + caption + "\"");
        }
        for (int i = 0; i < attributeIds.size(); i++)
        {
            const AttributeMapping::TagValue* tagVal = resolveAttributeByIndex(i, false);
            s.append(" " + tagVal->tag + "=" + tagVal->value);
        }
        for (int i = 0; i < additionalAttributeIds.size(); i++)
        {
            const AttributeMapping::TagValue* tagVal = resolveAttributeByIndex(i, true);
            s.append(" " + tagVal->tag + "=" + tagVal->value);
        }
    }
    return s;    
}
