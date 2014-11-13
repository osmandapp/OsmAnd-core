#include "MapObject.h"

#include "stdlib_common.h"
#include <algorithm>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "OsmAndCore.h"
#include "Common.h"
#include "QKeyIterator.h"

std::shared_ptr<const OsmAnd::MapObject::EncodingDecodingRules> OsmAnd::MapObject::defaultEncodingDecodingRules(OsmAnd::modifyAndReturn(
    std::shared_ptr<OsmAnd::MapObject::EncodingDecodingRules>(new OsmAnd::MapObject::EncodingDecodingRules()),
    (std::function<void(std::shared_ptr<OsmAnd::MapObject::EncodingDecodingRules>& instance)>)[]
    (std::shared_ptr<OsmAnd::MapObject::EncodingDecodingRules>& rules) -> void
    {
        rules->verifyRequiredRulesExist();
    }));

OsmAnd::MapObject::MapObject()
    : encodingDecodingRules(MapObject::defaultEncodingDecodingRules)
    , isArea(false)
{
}

OsmAnd::MapObject::~MapObject()
{
}

bool OsmAnd::MapObject::obtainSharingKey(SharingKey& outKey) const
{
    Q_UNUSED(outKey);
    return false;
}

bool OsmAnd::MapObject::obtainSortingKey(SortingKey& outKey) const
{
    Q_UNUSED(outKey);
    return false;
}

OsmAnd::ZoomLevel OsmAnd::MapObject::getMinZoomLevel() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapObject::getMaxZoomLevel() const
{
    return MaxZoomLevel;
}

QString OsmAnd::MapObject::toString() const
{
    return QString().sprintf("(@%p)", this);
}

bool OsmAnd::MapObject::isClosedFigure(bool checkInner /*= false*/) const
{
    if (checkInner)
    {
        for (const auto& polygon : constOf(innerPolygonsPoints31))
        {
            if (polygon.isEmpty())
                continue;

            if (polygon.first() != polygon.last())
                return false;
        }
        return true;
    }
    else
        return (points31.first() == points31.last());
}

void OsmAnd::MapObject::computeBBox31()
{
    bbox31.top() = bbox31.left() = std::numeric_limits<int32_t>::max();
    bbox31.bottom() = bbox31.right() = 0;

    auto pPoint31 = points31.constData();
    const auto pointsCount = points31.size();
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint31++)
        bbox31.enlargeToInclude(*pPoint31);
}

bool OsmAnd::MapObject::intersects(const AreaI& area) const
{
    // Check if area intersects bbox31 or bbox31 contains area or area contains bbox31
    // Fast check to exclude obviously false cases
    if (!bbox31.contains(area) && !area.contains(bbox31) && !area.intersects(bbox31))
        return false;

    // Check if any of the object points is inside area
    auto pPoint31 = points31.constData();
    const auto pointsCount = points31.size();
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint31++)
    {
        if (area.contains(*pPoint31))
            return true;
    }

    return false;
}

bool OsmAnd::MapObject::containsFoundationType() const
{
    return
        containsType(encodingDecodingRules->naturalCoastline_encodingRuleId) ||
        containsType(encodingDecodingRules->naturalCoastlineBroken_encodingRuleId) ||
        containsType(encodingDecodingRules->naturalCoastlineLine_encodingRuleId) ||
        containsType(encodingDecodingRules->naturalLand_encodingRuleId);
}

bool OsmAnd::MapObject::containsType(const uint32_t typeRuleId, bool checkAdditional /*= false*/) const
{
    return (checkAdditional ? additionalTypesRuleIds : typesRuleIds).contains(typeRuleId);
}

bool OsmAnd::MapObject::containsTypeSlow(const QString& tag, const QString& value, bool checkAdditional /*= false*/) const
{
    const auto citByTag = encodingDecodingRules->encodingRuleIds.constFind(tag);
    if (citByTag == encodingDecodingRules->encodingRuleIds.cend())
        return false;

    const auto citByValue = citByTag->constFind(value);
    if (citByValue == citByTag->cend())
        return false;

    const auto typeRuleId = *citByValue;
    return (checkAdditional ? additionalTypesRuleIds : typesRuleIds).contains(typeRuleId);
}

bool OsmAnd::MapObject::obtainTagValueByTypeRuleIndex(const uint32_t typeRuleIndex, QString& outTag, QString& outValue, bool checkAdditional /*= false*/) const
{
    const auto& rulesIds = checkAdditional ? additionalTypesRuleIds : typesRuleIds;

    if (typeRuleIndex >= rulesIds.size())
        return false;
    const auto typeRuleId = rulesIds[typeRuleIndex];

    const auto citRule = encodingDecodingRules->decodingRules.constFind(typeRuleId);
    if (citRule == encodingDecodingRules->decodingRules.cend())
        return false;

    const auto& rule = *citRule;
    outTag = rule.tag;
    outValue = rule.value;

    return true;
}

OsmAnd::MapObject::LayerType OsmAnd::MapObject::getLayerType() const
{
    return LayerType::Zero;
}

QString OsmAnd::MapObject::getCaptionInNativeLanguage() const
{
    const auto citName = captions.constFind(encodingDecodingRules->name_encodingRuleId);
    if (citName == captions.cend())
        return QString::null;
    return *citName;
}

QString OsmAnd::MapObject::getCaptionInLanguage(const QString& lang) const
{
    const auto citNameId = encodingDecodingRules->localizedName_encodingRuleIds.constFind(lang);
    if (citNameId == encodingDecodingRules->localizedName_encodingRuleIds.cend())
        return QString::null;

    const auto citName = captions.constFind(*citNameId);
    if (citName == captions.cend())
        return QString::null;
    return *citName;
}

OsmAnd::MapObject::EncodingDecodingRules::EncodingDecodingRules()
    : name_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalCoastline_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalLand_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineBroken_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineLine_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , highway_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , oneway_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , onewayReverse_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , layerLowest_encodingRuleId(std::numeric_limits<uint32_t>::max())
{
}

OsmAnd::MapObject::EncodingDecodingRules::~EncodingDecodingRules()
{
}

void OsmAnd::MapObject::EncodingDecodingRules::verifyRequiredRulesExist()
{
    uint32_t lastUsedRuleId = 0u;
    if (!decodingRules.isEmpty())
        lastUsedRuleId = *qMaxElement(keysOf(constOf(decodingRules)));

    createRequiredRules(lastUsedRuleId);
}

void OsmAnd::MapObject::EncodingDecodingRules::createRequiredRules(uint32_t& lastUsedRuleId)
{
    if (name_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("name"), QString::null);
    }

    if (naturalCoastline_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("natural"), QLatin1String("coastline"));
    }

    if (naturalLand_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("natural"), QLatin1String("land"));
    }

    if (naturalCoastlineBroken_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("natural"), QLatin1String("coastline_broken"));
    }

    if (naturalCoastlineLine_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("natural"), QLatin1String("coastline_line"));
    }

    if (highway_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("highway"), QLatin1String("yes"));
    }

    if (oneway_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("oneway"), QLatin1String("yes"));
    }

    if (onewayReverse_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("oneway"), QLatin1String("-1"));
    }

    if (layerLowest_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("layer"), QString::number(std::numeric_limits<int32_t>::min()));
    }
}

void OsmAnd::MapObject::EncodingDecodingRules::addRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue)
{
    // Insert encoding rule
    auto itEncodingRule = encodingRuleIds.find(ruleTag);
    if (itEncodingRule == encodingRuleIds.end())
        itEncodingRule = encodingRuleIds.insert(ruleTag, QHash<QString, uint32_t>());
    itEncodingRule->insert(ruleValue, ruleId);

    // Insert decoding rule
    if (!decodingRules.contains(ruleId))
    {
        DecodingRule rule;
        rule.tag = ruleTag;
        rule.value = ruleValue;

        decodingRules.insert(ruleId, rule);
    }

    // Capture quick-access rules
    if (QLatin1String("name") == ruleTag)
    {
        name_encodingRuleId = ruleId;
        namesRuleId.insert(ruleId);
    }
    else if (ruleTag.startsWith(QLatin1String("name:")))
    {
        const QString languageId = ruleTag.mid(QLatin1String("name:").size());
        localizedName_encodingRuleIds.insert(languageId, ruleId);
        localizedName_decodingRules.insert(ruleId, languageId);
        namesRuleId.insert(ruleId);
    }
    else if (QLatin1String("natural") == ruleTag && QLatin1String("coastline") == ruleValue)
        naturalCoastline_encodingRuleId = ruleId;
    else if (QLatin1String("natural") == ruleTag && QLatin1String("land") == ruleValue)
        naturalLand_encodingRuleId = ruleId;
    else if (QLatin1String("natural") == ruleTag && QLatin1String("coastline_broken") == ruleValue)
        naturalCoastlineBroken_encodingRuleId = ruleId;
    else if (QLatin1String("natural") == ruleTag && QLatin1String("coastline_line") == ruleValue)
        naturalCoastlineLine_encodingRuleId = ruleId;
    else if (QLatin1String("highway") == ruleTag && QLatin1String("yes") == ruleValue)
        highway_encodingRuleId = ruleId;
    else if (QLatin1String("oneway") == ruleTag && QLatin1String("yes") == ruleValue)
        oneway_encodingRuleId = ruleId;
    else if (QLatin1String("oneway") == ruleTag && QLatin1String("-1") == ruleValue)
        onewayReverse_encodingRuleId = ruleId;
}

OsmAnd::MapObject::EncodingDecodingRules::DecodingRule::DecodingRule()
{
}

OsmAnd::MapObject::EncodingDecodingRules::DecodingRule::~DecodingRule()
{
}

bool OsmAnd::MapObject::Comparator::operator()(const std::shared_ptr<const MapObject>& l, const std::shared_ptr<const MapObject>& r) const
{
    MapObject::SortingKey lKey;
    const auto lHasKey = l->obtainSortingKey(lKey);

    MapObject::SortingKey rKey;
    const auto rHasKey = r->obtainSortingKey(rKey);

    if (lHasKey && rHasKey)
    {
        if (lKey != rKey)
            return (lKey < rKey);

        return (l < r);
    }

    if (!lHasKey && !rHasKey)
        return (l < r);

    if (lHasKey && !rHasKey)
        return true;

    if (!lHasKey && rHasKey)
        return false;

    return (l < r);
}
