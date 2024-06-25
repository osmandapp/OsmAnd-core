#include "WorldRegions_P.h"
#include "WorldRegions.h"

#include <QFile>

#include "WorldRegion.h"
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
    QList< std::shared_ptr<const WorldRegion> >* const outRegions,
    const bool keepMapObjects,
    const AreaI* const bbox31,
    const VisitorFunction visitor,
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
        if (queryController && queryController->isAborted())
            break;

        auto keyNameAttributeId = std::numeric_limits<uint32_t>::max();
        auto downloadNameAttributeId = std::numeric_limits<uint32_t>::max();
        auto regionFullNameAttributeId = std::numeric_limits<uint32_t>::max();
        auto regionParentNameAttributeId = std::numeric_limits<uint32_t>::max();
        auto osmandRegionId = std::numeric_limits<uint32_t>::max();
        auto langAttributeId = std::numeric_limits<uint32_t>::max();
        auto metricAttributeId = std::numeric_limits<uint32_t>::max();
        auto roadSignsAttributeId = std::numeric_limits<uint32_t>::max();
        auto leftHandDrivingAttributeId = std::numeric_limits<uint32_t>::max();
        auto wikiLinkAttributeId = std::numeric_limits<uint32_t>::max();
        auto populationAttributeId = std::numeric_limits<uint32_t>::max();
        
        QHash<QString, std::shared_ptr<WorldRegion>> fullNamesToRegionData;
        QHash<QString, QList<std::shared_ptr<const OsmAnd::BinaryMapObject>>> unattachedBoundaryMapObjectsByRegions;

        bool attributesLocated = false;
        const auto worldRegionsCollector =
            [this,
                outRegions,
                keepMapObjects,
                visitor,
                &attributesLocated,
                &keyNameAttributeId,
                &downloadNameAttributeId,
                &regionFullNameAttributeId,
                &osmandRegionId,
                &regionParentNameAttributeId,
                &fullNamesToRegionData,
                &unattachedBoundaryMapObjectsByRegions,
                &langAttributeId,
                &metricAttributeId,
                &roadSignsAttributeId,
                &leftHandDrivingAttributeId,
                &wikiLinkAttributeId,
                &populationAttributeId]
            (const std::shared_ptr<const OsmAnd::BinaryMapObject>& mapObject) -> bool
            {
                if (!attributesLocated)
                {
                    const QString keyNameAttribute(QLatin1String("key_name"));
                    const QString downloadNameAttribute(QLatin1String("download_name"));
                    const QString regionFullNameAttribute(QLatin1String("region_full_name"));
                    const QString regionParentNameAttribute(QLatin1String("region_parent_name"));
                    const QString osmandRegionAttribute(QLatin1String("osmand_region"));
                    
                    const QString fieldLangAttribute(QLatin1String("region_lang"));
                    const QString fieldMetricAttribute(QLatin1String("region_metric"));
                    const QString fieldRoadSignsAttribute(QLatin1String("region_road_signs"));
                    const QString fieldLeftHandDrivingAttribute(QLatin1String("region_left_hand_navigation"));
                    const QString fieldWikiLinkAttribute(QLatin1String("region_wiki_link"));
                    const QString fieldPopulationAttribute(QLatin1String("region_population"));

                    const auto& encodeMap = mapObject->attributeMapping->encodeMap;
                    auto citAttributes = encodeMap.cend();

                    if ((citAttributes = encodeMap.constFind(&keyNameAttribute)) != encodeMap.cend())
                        keyNameAttributeId = citAttributes->constBegin().value();

                    if ((citAttributes = encodeMap.constFind(&downloadNameAttribute)) != encodeMap.cend())
                        downloadNameAttributeId = citAttributes->constBegin().value();

                    if ((citAttributes = encodeMap.constFind(&regionFullNameAttribute)) != encodeMap.cend())
                        regionFullNameAttributeId = citAttributes->constBegin().value();

                    if ((citAttributes = encodeMap.constFind(&regionParentNameAttribute)) != encodeMap.cend())
                        regionParentNameAttributeId = citAttributes->constBegin().value();
                    
                    if ((citAttributes = encodeMap.constFind(&osmandRegionAttribute)) != encodeMap.cend())
                        osmandRegionId = citAttributes->constBegin().value();

                    if ((citAttributes = encodeMap.constFind(&fieldLangAttribute)) != encodeMap.cend())
                        langAttributeId = citAttributes->constBegin().value();
                    
                    if ((citAttributes = encodeMap.constFind(&fieldMetricAttribute)) != encodeMap.cend())
                        metricAttributeId = citAttributes->constBegin().value();
                    
                    if ((citAttributes = encodeMap.constFind(&fieldRoadSignsAttribute)) != encodeMap.cend())
                        roadSignsAttributeId = citAttributes->constBegin().value();
                    
                    if ((citAttributes = encodeMap.constFind(&fieldLeftHandDrivingAttribute)) != encodeMap.cend())
                        leftHandDrivingAttributeId = citAttributes->constBegin().value();
                    
                    if ((citAttributes = encodeMap.constFind(&fieldWikiLinkAttribute)) != encodeMap.cend())
                        wikiLinkAttributeId = citAttributes->constBegin().value();
                    
                    if ((citAttributes = encodeMap.constFind(&fieldPopulationAttribute)) != encodeMap.cend())
                        populationAttributeId = citAttributes->constBegin().value();

                    attributesLocated = true;
                }

                auto worldRegion = std::make_shared<WorldRegion>();
                worldRegion->boundary = mapObject->containsAttribute("osmand_region", "boundary");
                worldRegion->regionMap = mapObject->containsAttribute("region_map", "yes", true);
                worldRegion->regionRoads = mapObject->containsAttribute("region_roads", "yes", true);
                worldRegion->regionJoinMap = mapObject->containsAttribute("region_join_map", "yes", true);
                worldRegion->regionJoinRoads = mapObject->containsAttribute("region_join_roads", "yes", true);
                if (worldRegion->boundary)
                {
                    const auto &fullRegionName = mapObject->captions[regionFullNameAttributeId];
                    const auto it = fullNamesToRegionData.find(fullRegionName);
                    if (it != fullNamesToRegionData.end())
                    {
                        addPolygonToRegionIfValid(mapObject, it.value());
                    }
                    else
                    {
                        const auto it = unattachedBoundaryMapObjectsByRegions.find(fullRegionName);
                        QList<std::shared_ptr<const OsmAnd::BinaryMapObject>> unattachedMapObjects;
                        if (it == unattachedBoundaryMapObjectsByRegions.end())
                            unattachedBoundaryMapObjectsByRegions[fullRegionName] = unattachedMapObjects;
                        else
                            unattachedMapObjects = it.value();
                        
                        unattachedMapObjects << mapObject;
                    }
                    return false;
                }
                for (const auto& captionEntry : rangeOf(constOf(mapObject->captions)))
                {
                    const auto& attributeId = captionEntry.key();
                    const auto& value = captionEntry.value();

                    if (attributeId == keyNameAttributeId)
                    {
                        worldRegion->regionName = value;
                    }
                    else if (attributeId == downloadNameAttributeId)
                    {
                        worldRegion->downloadName = value;
                    }
                    else if (attributeId == regionFullNameAttributeId)
                    {
                        worldRegion->fullRegionName = value;
                    }
                    else if (attributeId == regionParentNameAttributeId)
                    {
                        worldRegion->parentRegionName = value;
                    }
                    else if (attributeId == langAttributeId)
                    {
                        worldRegion->regionLang = value;
                    }
                    else if (attributeId == leftHandDrivingAttributeId)
                    {
                        worldRegion->regionLeftHandDriving = value;
                    }
                    else if (attributeId == metricAttributeId)
                    {
                        worldRegion->regionMetric = value;
                    }
                    else if (attributeId == roadSignsAttributeId)
                    {
                        worldRegion->regionRoadSigns = value;
                    }
                    else if (attributeId == wikiLinkAttributeId)
                    {
                        worldRegion->wikiLink = value;
                    }
                    else if (attributeId == populationAttributeId)
                    {
                        worldRegion->population = value;
                    }
                }
                worldRegion->nativeName = mapObject->getCaptionInNativeLanguage();
                worldRegion->localizedNames = mapObject->getCaptionsInAllLanguages();
                double cx = 0, cy = 0;
                const auto& points31 = mapObject->points31;
                for (int i = 0; i < points31.size(); i++)
                {
                    cx += points31[i].x;
                    cy += points31[i].y;
                }
                if (!points31.empty())
                {
                    cx /= points31.size();
                    cy /= points31.size();
                    worldRegion->regionCenter = LatLon(OsmAnd::Utilities::get31LatitudeY(cy), OsmAnd::Utilities::get31LongitudeX(cx));
                    worldRegion->polygon = mapObject->points31;
                }
                if (keepMapObjects)
                    worldRegion->mapObject = mapObject;
                
                if (worldRegion->fullRegionName.isEmpty())
                    return false;
                
                const auto it = unattachedBoundaryMapObjectsByRegions.find(worldRegion->fullRegionName);
                if (it != unattachedBoundaryMapObjectsByRegions.end())
                {
                    auto &unattachedMapObjects = it.value();
                    for (const auto &mapObject : unattachedMapObjects)
                    {
                        addPolygonToRegionIfValid(mapObject, worldRegion);
                    }
                    unattachedBoundaryMapObjectsByRegions.remove(worldRegion->fullRegionName);
                }

                if (!visitor || visitor(worldRegion))
                {
                    fullNamesToRegionData.insert(worldRegion->fullRegionName, worldRegion);
                    if (outRegions)
                        outRegions->push_back(qMove(worldRegion));
                }

                return false;
            };

        // Read objects from each map section
        OsmAnd::ObfMapSectionReader::loadMapObjects(
            ocbfReader,
            mapSection,
            mapSection->levels.first()->minZoom,
            bbox31, // Query entire world
            nullptr, // No need for map objects to be stored
            nullptr, // Surface type is not needed
            nullptr, // No filtering by ID
            worldRegionsCollector,
            nullptr, // No cache
            nullptr, // No cache
            queryController);
    }

    ocbfFile->close();

    if (queryController && queryController->isAborted())
        return false;
    return true;
}

void OsmAnd::WorldRegions_P::addPolygonToRegionIfValid(const std::shared_ptr<const OsmAnd::BinaryMapObject>& mapObject, const std::shared_ptr<WorldRegion> &worldRegion) const
{
    if (mapObject->points31.count() < 3)
        return;
    
    QVector<PointI> polygon = mapObject->points31;
    
    bool outside = true;
    for (const auto &point : polygon)
    {
        if (Utilities::isPointInsidePolygon(point, worldRegion->polygon)) {
            outside = false;
            break;
        }
    }
    
    if (outside)
        worldRegion->additionalPolygons << polygon;
}
