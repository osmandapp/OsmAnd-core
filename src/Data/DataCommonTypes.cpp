#include "DataCommonTypes.h"

#include "ObfSectionInfo.h"

static const int SHIFT_MULTIPOLYGON_IDS = 43;
static const int SHIFT_NON_SPLIT_EXISTING_IDS = 41;
static const uint64_t RELATION_BIT = 1L << (SHIFT_MULTIPOLYGON_IDS - 1); //According IndexPoiCreator SHIFT_MULTIPOLYGON_IDS
static const uint64_t SPLIT_BIT = 1L << (SHIFT_NON_SPLIT_EXISTING_IDS - 1); //According IndexVectorMapCreator
static const uint64_t clearBits = RELATION_BIT | SPLIT_BIT;
static const int DUPLICATE_SPLIT = 5; //According IndexPoiCreator DUPLICATE_SPLIT
static const int SHIFT_ID = 6;
static const int AMENITY_ID_RIGHT_SHIFT = 1;

OsmAnd::ObfObjectId OsmAnd::ObfObjectId::generateUniqueId(
    const uint64_t rawId,
    const uint32_t offsetInObf,
    const std::shared_ptr<const ObfSectionInfo>& obfSectionInfo)
{
    ObfObjectId uniqueId;
    uniqueId.id = rawId;

    // In case object originates from OSM, it should be unique
    if (uniqueId.isOsmId())
        return uniqueId;

    return generateUniqueId(offsetInObf, obfSectionInfo);
}

OsmAnd::ObfObjectId OsmAnd::ObfObjectId::generateUniqueId(
    const uint32_t offsetInObf,
    const std::shared_ptr<const ObfSectionInfo>& obfSectionInfo)
{
    ObfObjectId uniqueId;

    // Raw IDs that are negative should have been generated unique inside own section, but due to bug in original generator
    // this is not guaranteed in some cases. Thus generate new ID using OBF section runtime generated ID and offset inside OBF
    const uint64_t generatedId = (static_cast<uint64_t>(obfSectionInfo->runtimeGeneratedId) << 32) | static_cast<uint64_t>(offsetInObf);
    uniqueId.id = static_cast<uint64_t>(-static_cast<int64_t>(generatedId));

    return uniqueId;
}

QString OsmAnd::ObfObjectId::toString() const
{
    if (isOsmId())
    {
        return QString(QLatin1String("OSM-%1 %2 [%3]"))
            .arg((id & 0x1) ? "way" : "node")
            .arg(id >> 1)
            .arg(id);
    }

    const auto rawId = static_cast<uint64_t>(-static_cast<int64_t>(id));
    return QString(QLatin1String("OBF(%1):%2 [%3]"))
        .arg(static_cast<uint32_t>(rawId >> 32))
        .arg(static_cast<uint32_t>(rawId & 0xFFFFFFFF))
        .arg(id);
}

OsmAnd::ObfObjectId OsmAnd::ObfObjectId::fromRawId(const uint64_t rawId)
{
    if (static_cast<int64_t>(rawId) <= 0)
        return ObfObjectId::invalidId();

    ObfObjectId obfObjectId;
    obfObjectId.id = rawId;
    return obfObjectId;
}

int64_t OsmAnd::ObfObjectId::getOsmId() const
{
    uint64_t clearBits = RELATION_BIT | SPLIT_BIT;
    int64_t newId = isShiftedID() ? (id & ~clearBits) >> DUPLICATE_SPLIT : id;
    return newId >> SHIFT_ID;
}

bool OsmAnd::ObfObjectId::isShiftedID() const
{
    return isIdFromRelation() || isIdFromSplit();
}

bool OsmAnd::ObfObjectId::isIdFromRelation() const
{
    return id > 0 && (id & RELATION_BIT) == RELATION_BIT;
}

bool OsmAnd::ObfObjectId::isIdFromSplit() const
{
    return id > 0 && (id & SPLIT_BIT) == SPLIT_BIT;
}

int64_t OsmAnd::ObfObjectId::makeAmenityRightShift() const
{
    return id >> AMENITY_ID_RIGHT_SHIFT;
}
