#include "DataCommonTypes.h"

#include "ObfSectionInfo.h"

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
        return QString(QLatin1String("OSM-%1 %2"))
            .arg((id & 0x1) ? "way" : "node")
            .arg(id >> 1);
    }

    const auto rawId = static_cast<uint64_t>(-static_cast<int64_t>(id));
    return QString(QLatin1String("OBF(%1):%2"))
        .arg(static_cast<uint32_t>(rawId >> 32))
        .arg(static_cast<uint32_t>(rawId & 0xFFFFFFFF));
}
