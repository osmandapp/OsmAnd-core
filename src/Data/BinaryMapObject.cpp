#include "BinaryMapObject.h"

#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"

OsmAnd::BinaryMapObject::BinaryMapObject(
    const std::shared_ptr<const ObfMapSectionInfo>& section_,
    const std::shared_ptr<const ObfMapSectionLevel>& level_)
    : ObfMapObject(section_)
    , section(section_)
    , level(level_)
{
    encodingDecodingRules = section->getEncodingDecodingRules();
}

OsmAnd::BinaryMapObject::~BinaryMapObject()
{
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
    const auto& encDecRules = std::static_pointer_cast<const ObfMapSectionDecodingEncodingRules>(encodingDecodingRules);

    for (const auto& typeRuleId : constOf(additionalTypesRuleIds))
    {
        if (encDecRules->positiveLayers_encodingRuleIds.contains(typeRuleId))
            return LayerType::Positive;
        else if (encDecRules->negativeLayers_encodingRuleIds.contains(typeRuleId))
            return LayerType::Negative;
        else if (encDecRules->zeroLayers_encodingRuleIds.contains(typeRuleId))
            return LayerType::Zero;
    }

    return LayerType::Zero;
}
