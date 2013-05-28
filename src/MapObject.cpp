#include "MapObject.h"

#include "ObfMapSection.h"

OsmAnd::Model::MapObject::MapObject(ObfMapSection* section_)
    : section(section_)
    , id(_id)
    , names(_names)
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
        auto typeId = *itType;
        const auto& type = section->rules->decodingRules[typeId];

        const auto& key = std::get<0>(type);
        const auto& value = std::get<1>(type);

        if (key == "layer")
        {
            if(!value.isEmpty())
            {
                if(value[0] == '-')
                    return -1;
                else if (value[0] == '0')
                    return 0;
                else
                    return 1;
            }
        }
        else if (key == "tunnel")
        {
            isTunnel = (value == "yes");
        }
        else if (key == "bridge")
        {
            isBridge = (value == "yes");
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
        if(_polygonInnerCoordinates.isEmpty())
            return true;
        return _polygonInnerCoordinates.first() == _polygonInnerCoordinates.last();
    }
    else
        return _coordinates.first() == _coordinates.last();
}

bool OsmAnd::Model::MapObject::containsType( const QString& tag, const QString& value, bool checkAdditional /*= false*/ ) const
{
    uint32_t type;
    if(!section->rules->obtainTagValueId(tag, value, type))
        return false;

    return (checkAdditional ? _extraTypes : _extraTypes).contains(type);
}
