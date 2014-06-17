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
    const IQueryController* const controller) const
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
        if (controller && controller->isAborted())
        {
            ocbfFile->close();

            return false;
        }

        bool idsCaptured = false;
        uint32_t nameId = std::numeric_limits<uint32_t>::max();
        uint32_t idId = std::numeric_limits<uint32_t>::max();
        uint32_t regionPrefixId = std::numeric_limits<uint32_t>::max();
        uint32_t regionSuffixId = std::numeric_limits<uint32_t>::max();
        const QLatin1String localizedNameTagPrefix("name:");
        const auto localizedNameTagPrefixLen = localizedNameTagPrefix.size();
        const auto worldRegionsCollector =
            [&outRegions, &idsCaptured, &nameId, &idId, &regionPrefixId, &regionSuffixId, localizedNameTagPrefix, localizedNameTagPrefixLen]
            (const std::shared_ptr<const OsmAnd::Model::BinaryMapObject>& worldRegionMapObject) -> bool
            {
                const auto& rules = worldRegionMapObject->section->encodingDecodingRules;
                if (!idsCaptured)
                {
                    nameId = rules->name_encodingRuleId;

                    QHash< QString, QHash<QString, uint32_t> >::const_iterator citRule;
                    if ((citRule = rules->encodingRuleIds.constFind(QLatin1String("download_name"))) != rules->encodingRuleIds.cend())
                        idId = citRule->constBegin().value();
                    if ((citRule = rules->encodingRuleIds.constFind(QLatin1String("region_prefix"))) != rules->encodingRuleIds.cend())
                        regionPrefixId = citRule->constBegin().value();
                    if ((citRule = rules->encodingRuleIds.constFind(QLatin1String("region_suffix"))) != rules->encodingRuleIds.cend())
                        regionSuffixId = citRule->constBegin().value();

                    idsCaptured = true;
                }

                QString name;
                QString id;
                QString regionPrefix;
                QString regionSuffix;
                QHash<QString, QString> localizedNames;
                for(const auto& localizedNameEntry : rangeOf(constOf(worldRegionMapObject->names)))
                {
                    const auto ruleId = localizedNameEntry.key();
                    if (ruleId == nameId)
                    {
                        name = localizedNameEntry.value();
                        continue;
                    }
                    else if (ruleId == idId)
                    {
                        id = localizedNameEntry.value().toLower();
                        continue;
                    }
                    else if (ruleId == regionPrefixId)
                    {
                        regionPrefix = localizedNameEntry.value().toLower();
                        continue;
                    }
                    else if (ruleId == regionSuffixId)
                    {
                        regionSuffix = localizedNameEntry.value().toLower();
                        continue;
                    }

                    const auto& nameTag = rules->decodingRules[localizedNameEntry.key()].tag;
                    if (!nameTag.startsWith(localizedNameTagPrefix))
                        continue;
                    const auto languageId = nameTag.mid(localizedNameTagPrefixLen).toLower();
                    localizedNames.insert(languageId, localizedNameEntry.value());
                }

                QString parentId = regionSuffix;
                if (!regionPrefix.isEmpty())
                    parentId.prepend(regionPrefix + QLatin1String("_"));
                std::shared_ptr<const WorldRegion> newRegion(new WorldRegion(id, name, localizedNames, parentId));

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
            nullptr, // Foundation is not needed
            nullptr, // No filtering by ID
            worldRegionsCollector,
            controller);
    }

    ocbfFile->close();
    return true;
}
