#include "MapObject.h"

#include "ObfMapSectionReader.h"

OsmAnd::Model::MapObject::MapObject(const std::shared_ptr<const ObfMapSectionInfo>& section_)
    : _id(std::numeric_limits<uint64_t>::max())
    , _foundation(MapFoundationType::Unknown)
    , section(section_)
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
    for(auto itType = _extraTypes.begin(); itType != _extraTypes.end(); ++itType)
    {
        const auto& type = *itType;

        if (type.tag == "layer")
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
        else if (type.tag == "tunnel")
        {
            isTunnel = (type.value == "yes");
        }
        else if (type.tag == "bridge")
        {
            isBridge = (type.value == "yes");
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
        for(auto itPolygon = _innerPolygonsPoints31.begin(); itPolygon != _innerPolygonsPoints31.end(); ++itPolygon)
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
    for(auto itType = types.begin(); itType != types.end(); ++itType)
    {
        const auto& type = *itType;
        if(type.tag == tag && type.value == value)
            return true;
    }
    return false;
}

size_t OsmAnd::Model::MapObject::calculateApproxConsumedMemory() const
{
    size_t res = sizeof(MapObject) + _points31.size() * sizeof(PointI);
    for(auto itPolygon = _innerPolygonsPoints31.begin(); itPolygon != _innerPolygonsPoints31.end(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        res += polygon.size() * sizeof(PointI);
    }
    return res;
}
