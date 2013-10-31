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
    , types(_types)
    , extraTypes(_extraTypes)
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
    auto isTunnel = false;
    auto isBridge = false;
    for(auto itType = _extraTypes.cbegin(); itType != _extraTypes.cend(); ++itType)
    {
        const auto& type = *itType;

        if (type.tag == QLatin1String("layer"))
        {
            if(!type.value.isEmpty())
            {
                if(type.value[0] == '-')
                    return -1;
                else if (type.value[0] == '0')
                    return 0;
                else
                    return 1;
            }
        }
        else if (type.tag == QLatin1String("tunnel"))
        {
            isTunnel = (type.value == QLatin1String("yes"));
        }
        else if (type.tag == QLatin1String("bridge"))
        {
            isBridge = (type.value == QLatin1String("yes"));
        }
    }

    if (isTunnel)
        return -1;
    else if (isBridge)
        return 1;
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

bool OsmAnd::Model::MapObject::containsType( const QString& tag, const QString& value, bool checkAdditional /*= false*/ ) const
{
    const auto& types = (checkAdditional ? _extraTypes : _types);
    for(auto itType = types.cbegin(); itType != types.cend(); ++itType)
    {
        const auto& type = *itType;
        if(type.tag == tag && type.value == value)
            return true;
    }
    return false;
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
