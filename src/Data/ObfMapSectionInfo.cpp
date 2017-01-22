#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"

#include <OsmAndCore/Utilities.h>

OsmAnd::ObfMapSectionInfo::ObfMapSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , _p(new ObfMapSectionInfo_P(this))
{
}

OsmAnd::ObfMapSectionInfo::~ObfMapSectionInfo()
{
}

std::shared_ptr<const OsmAnd::ObfMapSectionAttributeMapping> OsmAnd::ObfMapSectionInfo::getAttributeMapping() const
{
    return _p->getAttributeMapping();
}

const OsmAnd::Nullable<OsmAnd::LatLon> OsmAnd::ObfMapSectionInfo::getCenterLatLon() const
{
    OsmAnd::Nullable<OsmAnd::LatLon> result;
    if (levels.isEmpty())
        return result;
    
    const auto mapRoot = levels.last();    
    result = OsmAnd::Utilities::convert31ToLatLon(mapRoot->area31.center());
    return result;
}

OsmAnd::ObfMapSectionLevel::ObfMapSectionLevel()
    : _p(new ObfMapSectionLevel_P(this))
    , firstDataBoxInnerOffset(0)
{
}

OsmAnd::ObfMapSectionLevel::~ObfMapSectionLevel()
{
}

OsmAnd::ObfMapSectionAttributeMapping::ObfMapSectionAttributeMapping()
    : tunnelAttributeId(std::numeric_limits<uint32_t>::max())
    , bridgeAttributeId(std::numeric_limits<uint32_t>::max())
{
    decodeMap.reserve(4096);

    positiveLayerAttributeIds.reserve(2);
    zeroLayerAttributeIds.reserve(1);
    negativeLayerAttributeIds.reserve(2);
}

OsmAnd::ObfMapSectionAttributeMapping::~ObfMapSectionAttributeMapping()
{
}

void OsmAnd::ObfMapSectionAttributeMapping::registerMapping(
    const uint32_t id,
    const QString& tag,
    const QString& value)
{
    MapObject::AttributeMapping::registerMapping(id, tag, value);

    if (QLatin1String("tunnel") == tag && QLatin1String("yes") == value)
    {
        tunnelAttributeId = id;
        negativeLayerAttributeIds.insert(id);
    }
    else if (QLatin1String("bridge") == tag && QLatin1String("yes") == value)
    {
        bridgeAttributeId = id;
        positiveLayerAttributeIds.insert(id);
    }
    else if (QLatin1String("layer") == tag)
    {
        if (!value.isEmpty() && value != QLatin1String("0"))
        {
            if (value[0] == QLatin1Char('-'))
                negativeLayerAttributeIds.insert(id);
            else if (value[0] == QLatin1Char('0'))
                zeroLayerAttributeIds.insert(id);
            else
                positiveLayerAttributeIds.insert(id);
        }
    }
}

void OsmAnd::ObfMapSectionAttributeMapping::registerRequiredMapping(uint32_t& lastUsedEntryId)
{
    MapObject::AttributeMapping::registerRequiredMapping(lastUsedEntryId);
}
