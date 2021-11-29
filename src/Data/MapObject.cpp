#include "MapObject.h"

#include "stdlib_common.h"
#include <algorithm>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "OsmAndCore.h"
#include "Common.h"
#include "QKeyIterator.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include <ICU.h>

static const int SHIFT_COORDINATES = 5;
static const int LABEL_ZOOM_ENCODE = 26;

std::shared_ptr<const OsmAnd::MapObject::AttributeMapping> OsmAnd::MapObject::defaultAttributeMapping(OsmAnd::modifyAndReturn(
    std::shared_ptr<OsmAnd::MapObject::AttributeMapping>(new OsmAnd::MapObject::AttributeMapping()),
    static_cast< std::function<void (std::shared_ptr<OsmAnd::MapObject::AttributeMapping>& instance)> >([]
    (std::shared_ptr<OsmAnd::MapObject::AttributeMapping>& mapping) -> void
    {
        mapping->verifyRequiredMappingRegistered();
    })));

OsmAnd::MapObject::MapObject()
    : attributeMapping(defaultAttributeMapping)
    , isArea(false)
    , labelX(0)
    , labelY(0)
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

bool OsmAnd::MapObject::intersectedOrContainedBy(const AreaI& area) const
{
    // Check if area intersects bbox31 or bbox31 contains area or area contains bbox31
    // Fast check to exclude obviously false cases
    if (!area.intersects(bbox31))
        return false;

    // Check if any of the object points is inside area
    auto pPoint31 = points31.constData();
    const auto pointsCount = points31.size();
    uint prevCross = 0;
    uint corners = 0;
    int lft = area.left() ;
    int rht = area.right() ;
    int tp = area.top() ;
    int btm = area.bottom() ;
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint31++)
    {
        uint cross = 0;
        int x31 = (*pPoint31).x;
        int y31 = (*pPoint31).y;
        cross |= (x31 < lft? 1 : 0);
        cross |= (x31 > rht? 2 : 0);
        cross |= (y31 < tp ? 4 : 0);
        cross |= (y31 > btm ? 8 : 0);
        if((pointIdx > 0 && (prevCross & cross) == 0) || cross == 0)
        {
            return true;
        }
        if(cross == (1 | 4)) {
            corners |= 1;
        } else if(cross == (2 | 4)) {
            corners |= 2;
        } else if(cross == (1 | 8)) {
            corners |= 4;
        } else if(cross == (2 | 8)) {
            corners |= 8;
        }
        prevCross = cross;
    }
    if(corners == 15) // && isArea - we can't here detect area or non-area field
        return true;
    return false;
}

bool OsmAnd::MapObject::containsAttribute(const uint32_t attributeId, const bool checkAdditional /*= false*/) const
{
    return (checkAdditional ? additionalAttributeIds : attributeIds).contains(attributeId);
}

bool OsmAnd::MapObject::containsAttribute(
    const QString& tag,
    const QString& value,
    const bool checkAdditional /*= false*/) const
{
    return containsAttribute(&tag, &value, checkAdditional);
}

bool OsmAnd::MapObject::containsAttribute(
    const QStringRef& tagRef,
    const QStringRef& valueRef,
    const bool checkAdditional /*= false*/) const
{
    const auto citTagsGroup = attributeMapping->encodeMap.constFind(tagRef);
    if (citTagsGroup == attributeMapping->encodeMap.cend())
        return false;

    const auto citAttributeId = citTagsGroup->constFind(valueRef);
    if (citAttributeId == citTagsGroup->cend())
        return false;

    return (checkAdditional ? additionalAttributeIds : attributeIds).contains(*citAttributeId);
}

bool OsmAnd::MapObject::containsTag(const QString& tag, const bool checkAdditional /*= false*/) const
{
    return containsTag(&tag, checkAdditional);
}

const OsmAnd::MapObject::AttributeMapping::TagValue* OsmAnd::MapObject::resolveAttributeByIndex(
    const uint32_t index,
    const bool additional /*= false*/) const
{
    const auto& ids = additional ? additionalAttributeIds : attributeIds;

    if (index >= ids.size())
        return nullptr;

    return attributeMapping->decodeMap.getRef(ids[index]);
}

bool OsmAnd::MapObject::containsTag(const QStringRef& tagRef, const bool checkAdditional /*= false*/) const
{
    const auto citTagsGroup = attributeMapping->encodeMap.constFind(tagRef);
    if (citTagsGroup == attributeMapping->encodeMap.cend())
        return false;

    for (const auto attributeId : constOf(*citTagsGroup))
    {
        if ((checkAdditional ? additionalAttributeIds : attributeIds).contains(attributeId))
            return true;
    }

    return false;
}

OsmAnd::MapObject::LayerType OsmAnd::MapObject::getLayerType() const
{
    return LayerType::Zero;
}

QString OsmAnd::MapObject::getCaptionInNativeLanguage() const
{
    const auto citName = captions.constFind(attributeMapping->nativeNameAttributeId);
    if (citName == captions.cend())
        return {};
    return *citName;
}

QString OsmAnd::MapObject::getCaptionInLanguage(const QString& lang) const
{
    const auto citNameAttributeId = attributeMapping->localizedNameAttributes.constFind(&lang);
    if (citNameAttributeId == attributeMapping->localizedNameAttributes.cend())
        return {};

    const auto citCaption = captions.constFind(*citNameAttributeId);
    if (citCaption == captions.cend())
        return {};
    return *citCaption;
}

QHash<QString, QString> OsmAnd::MapObject::getCaptionsInAllLanguages() const
{
    QHash<QString, QString> result;

    for (const auto& localizedNameAttributeEntry : rangeOf(constOf(attributeMapping->localizedNameAttributes)))
    {
        const auto& attributeId = localizedNameAttributeEntry.value();

        const auto citCaption = captions.constFind(attributeId);
        if (citCaption == captions.cend())
            continue;

        result.insert(localizedNameAttributeEntry.key().toString(), *citCaption);
    }

    return result;
}

QString OsmAnd::MapObject::getName(const QString lang, bool transliterate) const
{
    QString name = QString();
    if (!lang.isEmpty())
        name = getCaptionInLanguage(lang);
    
    if (name.isNull())
        name = getCaptionInNativeLanguage();
    
    if (transliterate && !name.isNull())
        return OsmAnd::ICU::transliterateToLatin(name);
    else
        return name;
}

OsmAnd::MapObject::AttributeMapping::AttributeMapping()
    : nativeNameAttributeId(std::numeric_limits<uint32_t>::max())
    , refAttributeId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineAttributeId(std::numeric_limits<uint32_t>::max())
    , naturalLandAttributeId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineBrokenAttributeId(std::numeric_limits<uint32_t>::max())
    , naturalCoastlineLineAttributeId(std::numeric_limits<uint32_t>::max())
    , onewayAttributeId(std::numeric_limits<uint32_t>::max())
    , onewayReverseAttributeId(std::numeric_limits<uint32_t>::max())
    , layerLowestAttributeId(std::numeric_limits<uint32_t>::max())
{
}

OsmAnd::MapObject::AttributeMapping::~AttributeMapping()
{
}

void OsmAnd::MapObject::AttributeMapping::verifyRequiredMappingRegistered()
{
    ListMap< TagValue >::KeyType maxKey = 0;
    decodeMap.findMaxKey(maxKey);

    auto lastUsedRuleId = static_cast<uint32_t>(maxKey);
    registerRequiredMapping(lastUsedRuleId);

    decodeMap.squeeze();
}


bool OsmAnd::MapObject::AttributeMapping::encodeTagValue(
    const QStringRef& tagRef,
    const QStringRef& valueRef,
    uint32_t* const pOutAttributeId /*= nullptr*/) const
{
    const auto citTagsGroup = encodeMap.constFind(tagRef);
    if (citTagsGroup == encodeMap.cend())
        return false;
    const auto citEncode = citTagsGroup->constFind(valueRef);
    if (citEncode == citTagsGroup->cend())
        return false;

    if (pOutAttributeId)
        *pOutAttributeId = *citEncode;

    return true;
}

bool OsmAnd::MapObject::AttributeMapping::encodeTagValue(
    const QString& tag,
    const QString& value,
    uint32_t* const pOutAttributeId /*= nullptr*/) const
{
    return encodeTagValue(&tag, &value, pOutAttributeId);
}

void OsmAnd::MapObject::AttributeMapping::registerRequiredMapping(uint32_t& lastUsedEntryId)
{
    if (nativeNameAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("name"), {});
    }

    if (refAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("ref"), {});
    }

    if (naturalCoastlineAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("natural"), QLatin1String("coastline"));
    }

    if (naturalLandAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("natural"), QLatin1String("land"));
    }

    if (naturalCoastlineBrokenAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("natural"), QLatin1String("coastline_broken"));
    }

    if (naturalCoastlineLineAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("natural"), QLatin1String("coastline_line"));
    }

    if (onewayAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("oneway"), QLatin1String("yes"));
    }

    if (onewayReverseAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("oneway"), QLatin1String("-1"));
    }

    if (layerLowestAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("layer"), QString::number(std::numeric_limits<int32_t>::min()));
    }
}

void OsmAnd::MapObject::AttributeMapping::registerMapping(
    const uint32_t id,
    const QString& tag,
    const QString& value)
{
    // Create decode mapping
    auto pDecode = decodeMap.getRef(id);
    if (pDecode)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Decode attribute %s = %s (#%u) already defined. Definition of %s = %s ignored.",
            qPrintable(pDecode->tag),
            qPrintable(pDecode->value),
            id,
            qPrintable(tag),
            qPrintable(value));
        return;
    }
    else
    {
        TagValue tagValueEntry;
        tagValueEntry.tag = tag;
        tagValueEntry.value = value;
        pDecode = decodeMap.insert(id, tagValueEntry);
    }
    const QStringRef tagRef(&pDecode->tag);
    const QStringRef valueRef(&pDecode->value);

    // Insert encoding rule
    auto itTagsGroup = encodeMap.find(tagRef);
    if (itTagsGroup == encodeMap.end())
        itTagsGroup = encodeMap.insert(tagRef, QHash<QStringRef, uint32_t>());
    auto itEncode = itTagsGroup->find(valueRef);
    if (itEncode != itTagsGroup->end())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Encode attribute %s = %s already has assigned identifier #%u. Redefinition to #%u ignored.",
            qPrintable(pDecode->tag),
            qPrintable(pDecode->value),
            *itEncode,
            id);
        return;
    }
    else
        itEncode = itTagsGroup->insert(valueRef, id);

    // Capture quick-access rules
    if (QLatin1String("name") == tag)
    {
        nativeNameAttributeId = id;
        nameAttributeIds.insert(id);
    }
    else if (tag.startsWith(QLatin1String("name:")))
    {
        const auto languageIdRef = tagRef.mid(QLatin1String("name:").size());
        localizedNameAttributes.insert(languageIdRef, id);
        localizedNameAttributeIds.insert(id, languageIdRef);
        nameAttributeIds.insert(id);
    }
    else if (QLatin1String("ref") == tag)
        refAttributeId = id;
    else if (QLatin1String("natural") == tag && QLatin1String("coastline") == value)
        naturalCoastlineAttributeId = id;
    else if (QLatin1String("natural") == tag && QLatin1String("land") == value)
        naturalLandAttributeId = id;
    else if (QLatin1String("natural") == tag && QLatin1String("coastline_broken") == value)
        naturalCoastlineBrokenAttributeId = id;
    else if (QLatin1String("natural") == tag && QLatin1String("coastline_line") == value)
        naturalCoastlineLineAttributeId = id;
    else if (QLatin1String("oneway") == tag && QLatin1String("yes") == value)
        onewayAttributeId = id;
    else if (QLatin1String("oneway") == tag && QLatin1String("-1") == value)
        onewayReverseAttributeId = id;
}

OsmAnd::MapObject::AttributeMapping::TagValue::TagValue()
{
}

OsmAnd::MapObject::AttributeMapping::TagValue::~TagValue()
{
}

QString OsmAnd::MapObject::AttributeMapping::TagValue::toString() const
{
    return tag + QLatin1String("=") + value;
}

OsmAnd::MapObject::Comparator::Comparator()
{
}

bool OsmAnd::MapObject::Comparator::operator()(
    const std::shared_ptr<const MapObject>& l,
    const std::shared_ptr<const MapObject>& r) const
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

bool OsmAnd::MapObject::isLabelCoordinatesSpecified() const
{
    return (labelX != 0 || labelY != 0) && points31.size() > 0;
}

int32_t OsmAnd::MapObject::getLabelCoordinateX() const
{
        int64_t sum = 0;
        int32_t LABEL_SHIFT = 31 - LABEL_ZOOM_ENCODE;
        int32_t pointsCount = points31.size();
        auto pPoint = points31.constData();
        for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            sum += pPoint->x;
        }
        int32_t average = (int32_t)(((sum >> SHIFT_COORDINATES) / pointsCount) << (SHIFT_COORDINATES - LABEL_SHIFT));
        int32_t label31X = (average + labelX) << LABEL_SHIFT;
        return label31X;
}

int32_t OsmAnd::MapObject::getLabelCoordinateY() const
{
        int64_t sum = 0;
        int32_t LABEL_SHIFT = 31 - LABEL_ZOOM_ENCODE;
        int32_t pointsCount = points31.size();
        auto pPoint = points31.constData();
        for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            sum += pPoint->y;
        }
        int32_t average = (int32_t)(((sum >> SHIFT_COORDINATES) / pointsCount) << (SHIFT_COORDINATES - LABEL_SHIFT));
        int32_t label31Y = (average + labelY) << LABEL_SHIFT;
        return label31Y;
}
