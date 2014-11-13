#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"

OsmAnd::ObfMapSectionInfo::ObfMapSectionInfo(const std::weak_ptr<ObfInfo>& owner)
    : ObfSectionInfo(owner)
    , _p(new ObfMapSectionInfo_P(this))
{
}

OsmAnd::ObfMapSectionInfo::~ObfMapSectionInfo()
{
}

std::shared_ptr<const OsmAnd::ObfMapSectionDecodingEncodingRules> OsmAnd::ObfMapSectionInfo::getEncodingDecodingRules() const
{
    return _p->getEncodingDecodingRules();
}

OsmAnd::ObfMapSectionLevel::ObfMapSectionLevel()
    : _p(new ObfMapSectionLevel_P(this))
    , firstDataBoxInnerOffset(0)
{
}

OsmAnd::ObfMapSectionLevel::~ObfMapSectionLevel()
{
}

OsmAnd::ObfMapSectionDecodingEncodingRules::ObfMapSectionDecodingEncodingRules()
    : ref_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , tunnel_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , bridge_encodingRuleId(std::numeric_limits<uint32_t>::max())
{
    positiveLayers_encodingRuleIds.reserve(2);
    zeroLayers_encodingRuleIds.reserve(1);
    negativeLayers_encodingRuleIds.reserve(2);
}

OsmAnd::ObfMapSectionDecodingEncodingRules::~ObfMapSectionDecodingEncodingRules()
{
}

uint32_t OsmAnd::ObfMapSectionDecodingEncodingRules::addRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue)
{
    MapObject::EncodingDecodingRules::addRule(ruleId, ruleTag, ruleValue);

    if (QLatin1String("ref") == ruleTag)
        ref_encodingRuleId = ruleId;
    else if (QLatin1String("tunnel") == ruleTag && QLatin1String("yes") == ruleValue)
    {
        tunnel_encodingRuleId = ruleId;
        negativeLayers_encodingRuleIds.insert(ruleId);
    }
    else if (QLatin1String("bridge") == ruleTag && QLatin1String("yes") == ruleValue)
    {
        bridge_encodingRuleId = ruleId;
        positiveLayers_encodingRuleIds.insert(ruleId);
    }
    else if (QLatin1String("layer") == ruleTag)
    {
        if (!ruleValue.isEmpty() && ruleValue != QLatin1String("0"))
        {
            if (ruleValue[0] == QLatin1Char('-'))
                negativeLayers_encodingRuleIds.insert(ruleId);
            else if (ruleValue[0] == QLatin1Char('0'))
                zeroLayers_encodingRuleIds.insert(ruleId);
            else
                positiveLayers_encodingRuleIds.insert(ruleId);
        }
    }

    return ruleId;
}

void OsmAnd::ObfMapSectionDecodingEncodingRules::createRequiredRules(uint32_t& lastUsedRuleId)
{
    MapObject::EncodingDecodingRules::createRequiredRules(lastUsedRuleId);
}
