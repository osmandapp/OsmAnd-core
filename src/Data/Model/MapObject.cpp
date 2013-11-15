#include "MapObject.h"

#include <cassert>

#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"

OsmAnd::Model::MapObject::MapObject(const std::shared_ptr<const ObfMapSectionInfo>& section_, const std::shared_ptr<const ObfMapSectionLevel>& level_)
    : _id(std::numeric_limits<uint64_t>::max())
    , _foundation(MapFoundationType::Undefined)
    , section(section_)
    , level(level_)
    , id(_id)
    , isArea(_isArea)
    , points31(_points31)
    , innerPolygonsPoints31(_innerPolygonsPoints31)
    , typesRuleIds(_typesRuleIds)
    , extraTypesRuleIds(_extraTypesRuleIds)
    , foundation(_foundation)
    , names(_names)
    , bbox31(_bbox31)
{
}

OsmAnd::Model::MapObject::~MapObject()
{
}

int OsmAnd::Model::MapObject::getSimpleLayerValue() const
{
    for(auto itTypeRuleId = _extraTypesRuleIds.cbegin(); itTypeRuleId != _extraTypesRuleIds.cend(); ++itTypeRuleId)
    {
        const auto typeRuleId = *itTypeRuleId;

        if(section->encodingDecodingRules->positiveLayers_encodingRuleIds.contains(typeRuleId))
            return 1;
        else if(section->encodingDecodingRules->negativeLayers_encodingRuleIds.contains(typeRuleId))
            return -1;
        else if(section->encodingDecodingRules->zeroLayers_encodingRuleIds.contains(typeRuleId))
            return 0;
    }

    return 0;
}

bool OsmAnd::Model::MapObject::isClosedFigure(bool checkInner /*= false*/) const
{
    if(checkInner)
    {
        for(auto itPolygon = _innerPolygonsPoints31.cbegin(); itPolygon != _innerPolygonsPoints31.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            if(polygon.isEmpty())
                continue;

            if(polygon.first() != polygon.last())
                return false;
        }
        return true;
    }
    else
        return _points31.first() == _points31.last();
}

bool OsmAnd::Model::MapObject::containsType(const uint32_t typeRuleId, bool checkAdditional /*= false*/) const
{
    return (checkAdditional ? _extraTypesRuleIds : _typesRuleIds).contains(typeRuleId);
}

bool OsmAnd::Model::MapObject::containsTypeSlow( const QString& tag, const QString& value, bool checkAdditional /*= false*/ ) const
{
    const auto typeRuleId = section->encodingDecodingRules->encodingRuleIds[tag][value];

    return (checkAdditional ? _extraTypesRuleIds : _typesRuleIds).contains(typeRuleId);
}

bool OsmAnd::Model::MapObject::intersects( const AreaI& area ) const
{
    // Check if any of the object points is inside area
    for(auto itPoint = _points31.cbegin(); itPoint != _points31.cend(); ++itPoint)
    {
        if(area.contains(*itPoint))
            return true;
    }

    // Check if area is inside map object
    if(bbox31.contains(area) || area.intersects(bbox31))
        return true;

    return false;
}

uint64_t OsmAnd::Model::MapObject::getUniqueId( const std::shared_ptr<const MapObject>& mapObject )
{
    return getUniqueId(mapObject->id, mapObject->section);
}

uint64_t OsmAnd::Model::MapObject::getUniqueId( const uint64_t id, const std::shared_ptr<const ObfMapSectionInfo>& section )
{
    uint64_t uniqueId = id;

    if(static_cast<int64_t>(uniqueId) < 0)
    {
        // IDs < 0 are guaranteed to be unique only inside own section
        const int64_t realId = -static_cast<int64_t>(uniqueId);
        assert((realId >> 32) == 0);
        assert(section);
        assert((section->runtimeGeneratedId >> 16) == 0);

        uniqueId = (static_cast<int64_t>(section->runtimeGeneratedId) << 32) | realId;
        uniqueId = static_cast<uint64_t>(-static_cast<int64_t>(uniqueId));
    }

    return uniqueId;
}
