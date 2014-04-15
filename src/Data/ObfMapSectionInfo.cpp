#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"

OsmAnd::ObfMapSectionInfo::ObfMapSectionInfo( const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , _p(new ObfMapSectionInfo_P(this))
    , _isBasemap(false)
    , isBasemap(_isBasemap)
    , levels(_levels)
    , encodingDecodingRules(_p->_encodingDecodingRules)
{
}

OsmAnd::ObfMapSectionInfo::ObfMapSectionInfo()
    : ObfSectionInfo(std::weak_ptr<ObfInfo>())
    , _p(new ObfMapSectionInfo_P(this))
    , _isBasemap(false)
    , isBasemap(_isBasemap)
    , levels(_levels)
    , encodingDecodingRules(_p->_encodingDecodingRules)
{
    auto rules = new ObfMapSectionDecodingEncodingRules();
    rules->createMissingRules();
    _p->_encodingDecodingRules.reset(rules);
}

OsmAnd::ObfMapSectionInfo::~ObfMapSectionInfo()
{
}

OsmAnd::ObfMapSectionLevel::ObfMapSectionLevel()
    : _p(new ObfMapSectionLevel_P(this))
    , offset(_offset)
    , length(_length)
    , minZoom(_minZoom)
    , maxZoom(_maxZoom)
    , area31(_area31)
{
}

OsmAnd::ObfMapSectionLevel::~ObfMapSectionLevel()
{
}

OsmAnd::ObfMapSectionDecodingEncodingRules::ObfMapSectionDecodingEncodingRules()
    : name_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , latinName_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , ref_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalCoastline_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalLand_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineBroken_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineLine_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , highway_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , oneway_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , onewayReverse_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , tunnel_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , bridge_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , layerLowest_encodingRuleId(std::numeric_limits<uint32_t>::max())
{
    positiveLayers_encodingRuleIds.reserve(2);
    zeroLayers_encodingRuleIds.reserve(1);
    negativeLayers_encodingRuleIds.reserve(2);
}

void OsmAnd::ObfMapSectionDecodingEncodingRules::createRule(const uint32_t ruleType, const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue)
{
    auto itEncodingRule = encodingRuleIds.find(ruleTag);
    if(itEncodingRule == encodingRuleIds.end())
        itEncodingRule = encodingRuleIds.insert(ruleTag, QHash<QString, uint32_t>());
    itEncodingRule->insert(ruleValue, ruleId);

    if(!decodingRules.contains(ruleId))
    {
        ObfMapSectionDecodingRule rule;
        rule.type = ruleType;
        rule.tag = ruleTag;
        rule.value = ruleValue;

        decodingRules.insert(ruleId, rule);
    }

    if(QLatin1String("name") == ruleTag)
        name_encodingRuleId = ruleId;
    else if(QLatin1String("name:en") == ruleTag)
        latinName_encodingRuleId = ruleId;
    else if(QLatin1String("ref") == ruleTag)
        ref_encodingRuleId = ruleId;
    else if(QLatin1String("natural") == ruleTag && QLatin1String("coastline") == ruleValue)
        naturalCoastline_encodingRuleId = ruleId;
    else if(QLatin1String("natural") == ruleTag && QLatin1String("land") == ruleValue)
        naturalLand_encodingRuleId = ruleId;
    else if(QLatin1String("natural") == ruleTag && QLatin1String("coastline_broken") == ruleValue)
        naturalCoastlineBroken_encodingRuleId = ruleId;
    else if(QLatin1String("natural") == ruleTag && QLatin1String("coastline_line") == ruleValue)
        naturalCoastlineLine_encodingRuleId = ruleId;
    else if(QLatin1String("oneway") == ruleTag && QLatin1String("yes") == ruleValue)
        oneway_encodingRuleId = ruleId;
    else if(QLatin1String("oneway") == ruleTag && QLatin1String("-1") == ruleValue)
        onewayReverse_encodingRuleId = ruleId;
    else if(QLatin1String("tunnel") == ruleTag && QLatin1String("yes") == ruleValue)
    {
        tunnel_encodingRuleId = ruleId;
        negativeLayers_encodingRuleIds.insert(ruleId);
    }
    else if(QLatin1String("bridge") == ruleTag && QLatin1String("yes") == ruleValue)
    {
        bridge_encodingRuleId = ruleId;
        positiveLayers_encodingRuleIds.insert(ruleId);
    }
    else if(QLatin1String("layer") == ruleTag)
    {
        if(!ruleValue.isEmpty() && ruleValue != QLatin1String("0"))
        {
            if(ruleValue[0] == '-')
                negativeLayers_encodingRuleIds.insert(ruleId);
            else if(ruleValue[0] == '0')
                zeroLayers_encodingRuleIds.insert(ruleId);
            else
                positiveLayers_encodingRuleIds.insert(ruleId);
        }
    }
}

void OsmAnd::ObfMapSectionDecodingEncodingRules::createMissingRules()
{
    auto nextId = decodingRules.size()*2 + 1;

    // Create 'name' encoding/decoding rule, if it still does not exist
    if(name_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        name_encodingRuleId = nextId++;
        createRule(0, name_encodingRuleId,
            QLatin1String("name"), QString::null);
    }

    // Create 'name:en' encoding/decoding rule, if it still does not exist
    if(latinName_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        latinName_encodingRuleId = nextId++;
        createRule(0, latinName_encodingRuleId,
            QLatin1String("name:en"), QString::null);
    }

    // Create 'natural=coastline' encoding/decoding rule, if it still does not exist
    if(naturalCoastline_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        naturalCoastline_encodingRuleId = nextId++;
        createRule(0, naturalCoastline_encodingRuleId,
            QLatin1String("natural"), QLatin1String("coastline"));
    }

    // Create 'natural=land' encoding/decoding rule, if it still does not exist
    if(naturalLand_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        naturalLand_encodingRuleId = nextId++;
        createRule(0, naturalLand_encodingRuleId,
            QLatin1String("natural"), QLatin1String("land"));
    }

    // Create 'natural=coastline_broken' encoding/decoding rule, if it still does not exist
    if(naturalCoastlineBroken_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        naturalCoastlineBroken_encodingRuleId = nextId++;
        createRule(0, naturalCoastlineBroken_encodingRuleId,
            QLatin1String("natural"), QLatin1String("coastline_broken"));
    }

    // Create 'natural=coastline_line' encoding/decoding rule, if it still does not exist
    if(naturalCoastlineLine_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        naturalCoastlineLine_encodingRuleId = nextId++;
        createRule(0, naturalCoastlineLine_encodingRuleId,
            QLatin1String("natural"), QLatin1String("coastline_line"));
    }

    // Create 'highway=yes' encoding/decoding rule, if it still does not exist
    if(highway_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        highway_encodingRuleId = nextId++;
        createRule(0, highway_encodingRuleId,
            QLatin1String("highway"), QLatin1String("yes"));
    }

    // Create 'layer=-int32_max' encoding/decoding rule, if it still does not exist
    if(layerLowest_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        layerLowest_encodingRuleId = nextId++;
        createRule(0, layerLowest_encodingRuleId,
            QLatin1String("layer"), QString::number(std::numeric_limits<int32_t>::min()));
    }
}
