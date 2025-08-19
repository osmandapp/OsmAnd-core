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
#include "Utilities.h"
#include <ICU.h>

static const int SHIFT_COORDINATES = 5;
static const int LABEL_ZOOM_ENCODE = 26;

static const int MIN_BBOX_FACTOR_TO_UPDATE_AREA = 2;
static const float MIN_INNER_DELTA_TO_UPDATE_AREA = 0.5;
static const float MIN_OUTER_DELTA_TO_UPDATE_AREA = 0.05;

OsmAnd::VisibleAreaPoints::VisibleAreaPoints(int64_t areaTime_, const AreaI& area31_, const QVector<PointI>& points31_)
    : areaTime(areaTime_)
    , area31(area31_)
    , points31(qMove(points31_))
{
}

OsmAnd::VisibleAreaPoints::~VisibleAreaPoints()
{
}

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
    , isCoastline(false)
    , vapIndex(0)
{
    vapItems[0] = nullptr;
    vapItems[1] = nullptr;
}

OsmAnd::MapObject::~MapObject()
{
    if (vapItems[0])
        delete vapItems[0];
    if (vapItems[1])
        delete vapItems[1];
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
    return QString::asprintf("(@%p)", this);
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

bool OsmAnd::MapObject::intersectedOrContainedBy(const AreaI& area,
    const AreaI& nextArea, int64_t nextAreaTime, QVector<PointI>* path31 /* = nullptr */) const
{
    // Check if area intersects bbox31 or bbox31 contains area or area contains bbox31
    // Fast check to exclude obviously false cases
    if (!area.intersects(bbox31))
        return false;

    bool result;

    auto areaIndex = startReadingArea();
    if (areaIndex >= 0)
    {
        if (vapItems[areaIndex]->area31.contains(area))
            result = intersectedOrContainedBy(vapItems[areaIndex]->points31, area, nextArea, nextAreaTime, nullptr);
        else
        {
            stopReadingArea(areaIndex);
            areaIndex = -1;
        }
    }
    if (areaIndex >= 0)
        stopReadingArea(areaIndex);
    else
        result = intersectedOrContainedBy(points31, area, nextArea, nextAreaTime, path31);

    return result;
}

bool OsmAnd::MapObject::intersectedOrContainedBy(const QVector<PointI>& points, const AreaI& area,
    const AreaI& nextArea, int64_t nextAreaTime, QVector<PointI>* path31 /* = nullptr */) const
{
    // Check if any of the object points is inside area
    auto pPoint31 = points.constData();
    const auto pointsCount = points.size();
    uint prevCross = 0;
    uint corners = 0;
    int lft = area.left() ;
    int rht = area.right() ;
    int tp = area.top() ;
    int btm = area.bottom() ;
    const auto left = nextArea.left();
    const auto right = nextArea.right();
    const auto top = nextArea.top();
    const auto bottom = nextArea.bottom();
    int x, y, prevX, prevY, code;
    int prevCode = 0;
    bool skipped = false;
    bool result = false;
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint31++)
    {
        uint cross = 0;
        x = (*pPoint31).x;
        y = (*pPoint31).y;
        cross |= (x < lft? 1 : 0);
        cross |= (x > rht? 2 : 0);
        cross |= (y < tp ? 4 : 0);
        cross |= (y > btm ? 8 : 0);
        if((pointIdx > 0 && (prevCross & cross) == 0) || cross == 0)
        {
            if (path31)
                result = true;
            else
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
        if (path31)
        {
            code = (x < left ? 1 : (x > right ? 2 : 0)) | (y < top ? 4 : (y > bottom ? 8 : 0));
            if (code != 0 && (code & prevCode) != 0)
                skipped = true;
            else
            {
                if (skipped)
                {
                    path31->resize(path31->size() + 1);
                    path31->last().x = prevX;
                    path31->last().y = prevY;
                    skipped = false;
                }
                path31->resize(path31->size() + 1);
                path31->last().x = x;
                path31->last().y = y;
                prevCode = code;
            }
            prevX = x;
            prevY = y;
        }
    }
    if(corners == 15) // && isArea - we can't here detect area or non-area field
        return true;
    return result;
}

inline int OsmAnd::MapObject::startReadingArea() const
{
    if (points31.size() < MIN_POINTS_TO_USE_SIMPLIFIED)
        return -1;

    const auto readIndex = vapIndex;

    if (!startReadingArea(vapCounts[readIndex]))
        return -1;

    return readIndex;
}

inline bool OsmAnd::MapObject::startReadingArea(QAtomicInt& a) const
{
    // If this area item can be read then increment the read count
    bool result = a.loadAcquire() > 0 &&
        (a.testAndSetOrdered(1, 2)
        || a.testAndSetOrdered(2, 3)
        || a.testAndSetOrdered(3, 4)
        || a.testAndSetOrdered(4, 5)
        || a.testAndSetOrdered(5, 6)
        || a.testAndSetOrdered(6, 7)
        || a.testAndSetOrdered(7, 8)
        || a.testAndSetOrdered(8, 9));

    return result;
}

inline void OsmAnd::MapObject::stopReadingArea(int index) const
{
    // Decrement the read count of this area and delete the item in case of zero
    if (vapCounts[index].fetchAndAddOrdered(-1) == 1)
    {
        const auto vapItem = vapItems[index];
        vapItems[index] = nullptr;
        delete vapItem;
    }
}

inline bool OsmAnd::MapObject::shouldChangeArea(const AreaI& prevArea, const AreaI& nextArea) const
{
    // Check if the new visible area is significantly different
    auto innerDeltaWidth = static_cast<int64_t>(prevArea.width() * MIN_INNER_DELTA_TO_UPDATE_AREA);
    auto outerDeltaWidth = static_cast<int64_t>(prevArea.width() * MIN_OUTER_DELTA_TO_UPDATE_AREA);
    auto innerDeltaHeight = static_cast<int64_t>(prevArea.height() * MIN_INNER_DELTA_TO_UPDATE_AREA);
    auto outerDeltaHeight = static_cast<int64_t>(prevArea.height() * MIN_OUTER_DELTA_TO_UPDATE_AREA);

    bool result = 
        nextArea.left() < static_cast<int64_t>(prevArea.left()) - outerDeltaWidth
        || nextArea.right() > static_cast<int64_t>(prevArea.right()) + outerDeltaWidth
        || nextArea.top() < static_cast<int64_t>(prevArea.top()) - outerDeltaWidth
        || nextArea.bottom() > static_cast<int64_t>(prevArea.bottom()) + outerDeltaWidth
        || nextArea.left() > static_cast<int64_t>(prevArea.left()) + innerDeltaWidth
        || nextArea.right() < static_cast<int64_t>(prevArea.right()) - innerDeltaWidth
        || nextArea.top() > static_cast<int64_t>(prevArea.top()) + innerDeltaWidth
        || nextArea.bottom() < static_cast<int64_t>(prevArea.bottom()) - innerDeltaWidth;

    return result;
}

bool OsmAnd::MapObject::needsSimplification(const AreaI& nextArea) const
{
    if (nextArea.isEmpty())
        return false;

    if (points31.size() < MIN_POINTS_TO_USE_SIMPLIFIED)
        return false;

    auto objectWidth = bbox31.width();
    auto objectHeight = bbox31.height();
    auto minWidth = static_cast<int64_t>(nextArea.width()) * MIN_BBOX_FACTOR_TO_UPDATE_AREA;
    auto minHeight = static_cast<int64_t>(nextArea.height()) * MIN_BBOX_FACTOR_TO_UPDATE_AREA;

    if (objectWidth < minWidth && objectHeight < minHeight)
        return false;

    return true;
}

bool OsmAnd::MapObject::updateVisibleArea(const AreaI& nextArea, int64_t nextAreaTime, QVector<PointI>* path31) const
{
    bool result = false;

    int64_t prevAreaTime;
    AreaI prevArea31;

    // Try to lock and update
    if (vapWriteLock.testAndSetOrdered(0, 1))
    {
        const auto readIndex = vapIndex;
        if (startReadingArea(vapCounts[readIndex]))
        {
            // Read and check visible area if it needs to be replaced
            prevAreaTime = vapItems[readIndex]->areaTime;
            prevArea31 = vapItems[readIndex]->area31;
            stopReadingArea(readIndex);
            if (nextAreaTime > prevAreaTime && shouldChangeArea(prevArea31, nextArea))
            {
                // Try to update previous visible area
                const auto writeIndex = 1 - readIndex;
                if (vapCounts[writeIndex].testAndSetOrdered(1, 0))
                {
                    prevAreaTime = vapItems[writeIndex]->areaTime;
                    prevArea31 = vapItems[writeIndex]->area31;
                    if (nextAreaTime > prevAreaTime && shouldChangeArea(prevArea31, nextArea))
                    {
                        auto nextPoints31 = path31 ? qMove(*path31)
                            :(points31.size() < MIN_POINTS_TO_USE_SIMPLIFIED ? points31
                                : Utilities::simplifyPathOutsideBBox(points31, nextArea));
                        if (nextPoints31.size() * MIN_BBOX_FACTOR_TO_UPDATE_AREA < points31.size())
                        {
                            delete vapItems[writeIndex];
                            vapItems[writeIndex] = new VisibleAreaPoints(nextAreaTime, nextArea, nextPoints31);
                        }
                    }
                    vapCounts[writeIndex].storeRelease(1);
                    vapIndex = writeIndex;
                    result = true;
                }
            }
        }
        vapWriteLock.storeRelease(0);
    }
    return result;
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

    if (valueRef.isEmpty())
    {
        // return true if contains any
        for (auto it = citTagsGroup->constBegin(); it != citTagsGroup->constEnd(); ++it)
        {
            if ((checkAdditional ? additionalAttributeIds : attributeIds).contains(it.value()))
            {
                return true;
            }
        }

        return false;
    }

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

QString OsmAnd::MapObject::formatWithAdditionalAttributes(const QString& value) const
{
    if (!value.contains('?'))
        return value;

    int startIndex = value.indexOf('?');
    int endIndex = value.lastIndexOf('?');

    QString formatted = value.left(startIndex);
    QString tag = value.mid(startIndex + 1, endIndex - startIndex - 1);
    for (uint32_t attrIdIndex = 0; attrIdIndex < additionalAttributeIds.size(); attrIdIndex++) {
        const auto& attribute = resolveAttributeByIndex(attrIdIndex, true);
        if (attribute && attribute->tag == tag)
        {
            formatted.append(attribute->value);
            break;
        }
    }
    formatted.append(value.mid(endIndex + 1));

    return formatted;
}

OsmAnd::MapObject::LayerType OsmAnd::MapObject::getLayerType() const
{
    return LayerType::Zero;
}

QString OsmAnd::MapObject::getCaptionInNativeLanguage() const
{
    const auto citName = captions.constFind(attributeMapping->nativeNameAttributeId);
    if (citName == captions.cend())
        return QString::null;
    return *citName;
}

QString OsmAnd::MapObject::getCaptionInLanguage(const QString& lang) const
{
    const auto citNameAttributeId = attributeMapping->localizedNameAttributes.constFind(&lang);
    if (citNameAttributeId == attributeMapping->localizedNameAttributes.cend())
        return QString::null;

    const auto citCaption = captions.constFind(*citNameAttributeId);
    if (citCaption == captions.cend())
        return QString::null;
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
    if (transliterate)
    {
        const auto& enCaption = getCaptionInLanguage(QStringLiteral("en"));
        if (!enCaption.isEmpty())
            return enCaption;
    }
    
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
    , enNameAttributeId(std::numeric_limits<uint32_t>::max())
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
            QLatin1String("name"), QString::null);
    }

    if (refAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("ref"), QString::null);
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
        if (tag == QLatin1String("name:en"))
            enNameAttributeId = id;
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

QString OsmAnd::MapObject::getResolvedAttribute(const QStringRef& tagRef) const
{
    QString result;

    const auto& citTagsGroup = attributeMapping->encodeMap.constFind(tagRef);
    if (citTagsGroup != attributeMapping->encodeMap.cend() && !citTagsGroup->empty())
    {
        for (const auto attributeId : constOf(*citTagsGroup))
        {
            if (attributeMapping->decodeMap.size() > attributeId &&
                (attributeIds.contains(attributeId) || additionalAttributeIds.contains(attributeId)))
            {
                result = attributeMapping->decodeMap[attributeId].value;
                break;
            }
        }
    }
    
    return result;
}

QList<QPair<QString, QString>> OsmAnd::MapObject::getResolvedAttributesListPairs() const
{
  QList<QPair<QString, QString>> result;
  for (const auto& attributeId : constOf(attributeIds))
  {
    if (attributeMapping->decodeMap.size() > attributeId) {
      const auto& decodedAttribute = attributeMapping->decodeMap[attributeId];
      result.append(qMakePair(decodedAttribute.tag, decodedAttribute.value));
    }
  }
  for (const auto& attributeId : constOf(additionalAttributeIds))
  {
    if (attributeMapping->decodeMap.size() > attributeId) {
      const auto& decodedAttribute = attributeMapping->decodeMap[attributeId];
      result.append(qMakePair(decodedAttribute.tag, decodedAttribute.value));
    }
  }
  for (const auto& captionAttributeId : constOf(captionsOrder))
  {
    if (captionAttributeId == attributeMapping->nativeNameAttributeId || attributeMapping->localizedNameAttributeIds.contains(captionAttributeId))
    {
      // caption is name or localized name:lang
      continue;
    }
    // caption is additional="text"
    const QString & caption = constOf(captions)[captionAttributeId];
    const QString & tag = attributeMapping->decodeMap[captionAttributeId].tag;
    result.append(qMakePair(tag, caption));
  }
  return result;
}

QHash<QString, QString> OsmAnd::MapObject::getResolvedAttributes() const
{
  QHash<QString, QString> result;
  QList<QPair<QString, QString>> listPairs = getResolvedAttributesListPairs();
  for (const auto &pair : listPairs) {
    result.insert(pair.first, pair.second);
  }
  return result;
}

QString OsmAnd::MapObject::debugInfo(long id, bool all) const
{
    return QString();
}
