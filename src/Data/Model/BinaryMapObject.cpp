#include "BinaryMapObject.h"

#include <cassert>

#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"

OsmAnd::Model::BinaryMapObject::BinaryMapObject(const std::shared_ptr<const ObfMapSectionInfo>& section_, const std::shared_ptr<const ObfMapSectionLevel>& level_)
    : section(section_)
    , level(level_)
    , isArea(_isArea)
    , points31(_points31)
    , innerPolygonsPoints31(_innerPolygonsPoints31)
    , typesRuleIds(_typesRuleIds)
    , extraTypesRuleIds(_extraTypesRuleIds)
    , captions(_captions)
    , captionsOrder(_captionsOrder)
    , bbox31(_bbox31)
{
}

OsmAnd::Model::BinaryMapObject::~BinaryMapObject()
{
}

int OsmAnd::Model::BinaryMapObject::getSimpleLayerValue() const
{
    for (const auto& typeRuleId : constOf(_extraTypesRuleIds))
    {
        if (section->encodingDecodingRules->positiveLayers_encodingRuleIds.contains(typeRuleId))
            return 1;
        else if (section->encodingDecodingRules->negativeLayers_encodingRuleIds.contains(typeRuleId))
            return -1;
        else if (section->encodingDecodingRules->zeroLayers_encodingRuleIds.contains(typeRuleId))
            return 0;
    }

    return 0;
}

bool OsmAnd::Model::BinaryMapObject::isClosedFigure(bool checkInner /*= false*/) const
{
    if (checkInner)
    {
        for (const auto& polygon : constOf(_innerPolygonsPoints31))
        {
            if (polygon.isEmpty())
                continue;

            if (polygon.first() != polygon.last())
                return false;
        }
        return true;
    }
    else
        return _points31.first() == _points31.last();
}

bool OsmAnd::Model::BinaryMapObject::containsType(const uint32_t typeRuleId, bool checkAdditional /*= false*/) const
{
    return (checkAdditional ? _extraTypesRuleIds : _typesRuleIds).contains(typeRuleId);
}

bool OsmAnd::Model::BinaryMapObject::containsTypeSlow(const QString& tag, const QString& value, bool checkAdditional /*= false*/) const
{
    const auto typeRuleId = section->encodingDecodingRules->encodingRuleIds[tag][value];

    return (checkAdditional ? _extraTypesRuleIds : _typesRuleIds).contains(typeRuleId);
}

bool OsmAnd::Model::BinaryMapObject::intersects(const AreaI& area) const
{
    // Check if any of the object points is inside area
    for (const auto& point : constOf(_points31))
    {
        if (area.contains(point))
            return true;
    }

    // Check if area is inside map object
    if (bbox31.contains(area) || area.intersects(bbox31))
        return true;

    return false;
}

QString OsmAnd::Model::BinaryMapObject::getNameInNativeLanguage() const
{
    const auto citName = _captions.constFind(section->encodingDecodingRules->name_encodingRuleId);
    if (citName == _captions.cend())
        return QString::null;
    return *citName;
}

QString OsmAnd::Model::BinaryMapObject::getNameInLanguage(const QString& lang) const
{
    const auto& encDecRules = section->encodingDecodingRules;
    const auto citNameId = encDecRules->localizedName_encodingRuleIds.constFind(lang);
    if (citNameId == encDecRules->localizedName_encodingRuleIds.cend())
        return QString::null;

    const auto citName = _captions.constFind(*citNameId);
    if (citName == _captions.cend())
        return QString::null;
    return *citName;
}

uint64_t OsmAnd::Model::BinaryMapObject::getUniqueId(const std::shared_ptr<const BinaryMapObject>& mapObject)
{
    return getUniqueId(mapObject->id, mapObject->section);
}

uint64_t OsmAnd::Model::BinaryMapObject::getUniqueId(const uint64_t id, const std::shared_ptr<const ObfMapSectionInfo>& section)
{
    uint64_t uniqueId = id;

    if (static_cast<int64_t>(uniqueId) < 0)
    {
        // IDs < 0 are guaranteed to be unique only inside own section
        const int64_t realId = -static_cast<int64_t>(uniqueId);
        assert((realId >> 48) == 0);
        assert(section);
        assert((section->runtimeGeneratedId >> 16) == 0);

        uniqueId = (static_cast<int64_t>(section->runtimeGeneratedId) << 48) | realId;
        uniqueId = static_cast<uint64_t>(-static_cast<int64_t>(uniqueId));
    }

    return uniqueId;
}
