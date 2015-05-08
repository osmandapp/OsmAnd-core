#include "WorldRegions_P.h"
#include "WorldRegions.h"

#include <QFile>

#include "ObfReader.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionReader.h"
#include "BinaryMapObject.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::WorldRegions_P::WorldRegions_P(WorldRegions* const owner_)
    : owner(owner_)
{
}

OsmAnd::WorldRegions_P::~WorldRegions_P()
{
}

bool OsmAnd::WorldRegions_P::loadWorldRegions(
    QHash< QString, std::shared_ptr<const WorldRegion> >& outRegions,
    const std::shared_ptr<const IQueryController>& queryController) const
{
    const std::shared_ptr<QIODevice> ocbfFile(new QFile(owner->ocbfFileName));
    if (!ocbfFile->open(QIODevice::ReadOnly))
        return false;

    const std::shared_ptr<ObfReader> ocbfReader(new ObfReader(ocbfFile));
    const auto& obfInfo = ocbfReader->obtainInfo();
    if (!obfInfo)
    {
        ocbfFile->close();

        return false;
    }

    for(const auto& mapSection : constOf(obfInfo->mapSections))
    {
        // Check if request is aborted
        if (queryController && queryController->isAborted())
        {
            ocbfFile->close();

            return false;
        }

        bool idsCaptured = false;
        uint32_t nameId = std::numeric_limits<uint32_t>::max();
        uint32_t idId = std::numeric_limits<uint32_t>::max();
        uint32_t downloadNameId = std::numeric_limits<uint32_t>::max();
        uint32_t regionPrefixId = std::numeric_limits<uint32_t>::max();
        uint32_t regionSuffixId = std::numeric_limits<uint32_t>::max();
        const QLatin1String localizedNameTagPrefix("name:");
        const auto localizedNameTagPrefixLen = localizedNameTagPrefix.size();
        const auto worldRegionsCollector =
            [&outRegions, &idsCaptured, &nameId, &idId, &downloadNameId, &regionPrefixId, &regionSuffixId, localizedNameTagPrefix, localizedNameTagPrefixLen]
            (const std::shared_ptr<const OsmAnd::BinaryMapObject>& worldRegionMapObject) -> bool
            {
                const QString keyNameTag(QLatin1String("key_name"));
                const QString downloadNameTag(QLatin1String("download_name"));
                const QString regionPrefixTag(QLatin1String("region_prefix"));
                const QString regionSuffixTag(QLatin1String("region_suffix"));

                const auto& encodeMap = worldRegionMapObject->attributeMapping->encodeMap;
                if (!idsCaptured)
                {
                    nameId = worldRegionMapObject->attributeMapping->nativeNameAttributeId;

                    QHash< QStringRef, QHash<QStringRef, uint32_t> >::const_iterator citAttributes;
                    if ((citAttributes = encodeMap.constFind(&keyNameTag)) != encodeMap.cend())
                        idId = citAttributes->constBegin().value();
                    if ((citAttributes = encodeMap.constFind(&downloadNameTag)) != encodeMap.cend())
                        downloadNameId = citAttributes->constBegin().value();
                    if ((citAttributes = encodeMap.constFind(&regionPrefixTag)) != encodeMap.cend())
                        regionPrefixId = citAttributes->constBegin().value();
                    if ((citAttributes = encodeMap.constFind(&regionSuffixTag)) != encodeMap.cend())
                        regionSuffixId = citAttributes->constBegin().value();

                    idsCaptured = true;
                }

                QString name;
                QString id;
                QString downloadName;
                QString regionPrefix;
                QString regionSuffix;
                QHash<QString, QString> localizedNames;
                for(const auto& captionEntry : rangeOf(constOf(worldRegionMapObject->captions)))
                {
                    const auto ruleId = captionEntry.key();
                    if (ruleId == idId)
                    {
                        id = captionEntry.value().toLower();
                        continue;
                    }
                    else if (ruleId == nameId)
                    {
                        name = captionEntry.value();
                        continue;
                    }
                    else if (ruleId == downloadNameId)
                    {
                        downloadName = captionEntry.value().toLower();
                        continue;
                    }
                    else if (ruleId == regionPrefixId)
                    {
                        regionPrefix = captionEntry.value().toLower();
                        continue;
                    }
                    else if (ruleId == regionSuffixId)
                    {
                        regionSuffix = captionEntry.value().toLower();
                        continue;
                    }

                    const auto& nameTag = worldRegionMapObject->attributeMapping->decodeMap[captionEntry.key()].tag;
                    if (!nameTag.startsWith(localizedNameTagPrefix))
                        continue;
                    const auto languageId = nameTag.mid(localizedNameTagPrefixLen).toLower();
                    localizedNames.insert(languageId, captionEntry.value());
                }

                QString parentId = regionSuffix;
                if (!regionPrefix.isEmpty())
                {
                    if (!parentId.isEmpty())
                        parentId.prepend(regionPrefix + QLatin1String("_"));
                    else
                        parentId = regionPrefix;
                }

                // Build up full id
                if (!regionPrefix.isEmpty())
                    id.prepend(regionPrefix + QLatin1String("_"));
                if (!regionSuffix.isEmpty())
                    id.append(QLatin1String("_") + regionSuffix);

                std::shared_ptr<const WorldRegion> newRegion(new WorldRegion(id, downloadName, name, localizedNames, parentId));

                outRegions.insert(id, qMove(newRegion));

                return false;
            };

        // Read objects from each map section
        OsmAnd::ObfMapSectionReader::loadMapObjects(
            ocbfReader,
            mapSection,
            mapSection->levels.first()->minZoom,
            nullptr, // Query entire world
            nullptr, // No need for map objects to be stored
            nullptr, // Surface type is not needed
            nullptr, // No filtering by ID
            worldRegionsCollector,
            nullptr, // No cache
            nullptr, // No cache
            queryController);
    }

    ocbfFile->close();
    return true;
}
