#include "ObfDataInterface.h"

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QSet>
#include <QHash>
#include <QList>
#include "restore_internal_warnings.h"

#include "Ref.h"
#include "ObfReader.h"
#include "ObfInfo.h"
#include "ObfMapSectionReader.h"
#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionReader.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfPoiSectionReader.h"
#include "ObfPoiSectionInfo.h"
#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionInfo.h"
#include "ObfTransportSectionReader.h"
#include "ObfTransportSectionInfo.h"
#include "ObfMapObject.h"
#include "Amenity.h"
#include "StreetGroup.h"
#include "Street.h"
#include "TransportStop.h"
#include "TransportRoute.h"
#include "IQueryController.h"
#include "FunctorQueryController.h"
#include "QKeyValueIterator.h"

OsmAnd::ObfDataInterface::ObfDataInterface(const QList< std::shared_ptr<const ObfReader> >& obfReaders_)
    : obfReaders(obfReaders_)
{
}

OsmAnd::ObfDataInterface::~ObfDataInterface()
{
}

bool OsmAnd::ObfDataInterface::loadObfFiles(
    QList< std::shared_ptr<const ObfFile> >* outFiles /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        // Initialize OBF file
        obfReader->obtainInfo();

        if (outFiles)
            outFiles->push_back(obfReader->obfFile);
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadBinaryMapObjects(
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
    MapSurfaceType* outSurfaceType,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfMapSectionReader::FilterByIdFunction filterById /*= nullptr*/,
    ObfMapSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric /*= nullptr*/)
{
    auto mergedSurfaceType = MapSurfaceType::Undefined;
    std::shared_ptr<const ObfReader> basemapReader;

    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();

        // Handle main basemap
        if (obfInfo->isBasemapWithCoastlines)
        {
            // In case there's more than 1 basemap reader present, use only first and warn about this fact
            if (basemapReader)
            {
                LogPrintf(LogSeverityLevel::Warning, "More than 1 basemap available");
                continue;
            }

            // Save basemap reader for later use
            basemapReader = obfReader;

            // In case requested zoom is more detailed than basemap max zoom, skip basemap processing for now
            if (zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
                continue;
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                obfReader,
                mapSection,
                zoom,
                bbox31,
                resultOut,
                &surfaceTypeToMerge,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                queryController,
                metric);
            if (surfaceTypeToMerge != MapSurfaceType::Undefined)
            {
                if (mergedSurfaceType == MapSurfaceType::Undefined)
                    mergedSurfaceType = surfaceTypeToMerge;
                else if (mergedSurfaceType != surfaceTypeToMerge)
                    mergedSurfaceType = MapSurfaceType::Mixed;
            }
        }
    }

    // In case there's basemap available and requested zoom is more detailed than basemap max zoom level,
    // read tile from MaxBasemapZoomLevel that covers requested tile
    if (basemapReader && zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
    {
        const auto& obfInfo = basemapReader->obtainInfo();

        // Calculate proper bbox31 on MaxBasemapZoomLevel (if possible)
        const AreaI *pBasemapBBox31 = nullptr;
        AreaI basemapBBox31;
        if (bbox31)
        {
            pBasemapBBox31 = &basemapBBox31;
            basemapBBox31 = Utilities::roundBoundingBox31(
                *bbox31,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel));
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                basemapReader,
                mapSection,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel),
                pBasemapBBox31,
                resultOut,
                &surfaceTypeToMerge,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                queryController,
                metric);

            // Basemap must always have a surface type defined
            assert(surfaceTypeToMerge != MapSurfaceType::Undefined);
            if (mergedSurfaceType == MapSurfaceType::Undefined)
                mergedSurfaceType = surfaceTypeToMerge;
            else if (mergedSurfaceType != surfaceTypeToMerge)
                mergedSurfaceType = MapSurfaceType::Mixed;
        }
    }

    // In case there was a basemap present, Undefined is Land
    if (mergedSurfaceType == MapSurfaceType::Undefined && !basemapReader)
        mergedSurfaceType = MapSurfaceType::FullLand;

    if (outSurfaceType)
        *outSurfaceType = mergedSurfaceType;

    return true;
}

bool OsmAnd::ObfDataInterface::loadRoads(
    const RoutingDataLevel dataLevel,
    const AreaI* const bbox31 /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::Road> >* resultOut /*= nullptr*/,
    const FilterRoadsByIdFunction filterById /*= nullptr*/,
    const ObfRoutingSectionReader::VisitorFunction visitor /*= nullptr*/,
    ObfRoutingSectionReader::DataBlocksCache* cache /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedCacheEntries /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& routingSection : constOf(obfInfo->routingSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            OsmAnd::ObfRoutingSectionReader::loadRoads(
                obfReader,
                routingSection,
                dataLevel,
                bbox31,
                resultOut,
                filterById,
                nullptr,
                cache,
                outReferencedCacheEntries,
                queryController,
                metric);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadRoutingTreeNodes(
    const std::shared_ptr<const ObfReader>& obfReader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& routingSection,
    const RoutingDataLevel dataLevel,
    QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> >* resultOut)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& routingSection : constOf(obfInfo->routingSections))
        {
            OsmAnd::ObfRoutingSectionReader::loadTreeNodes(
                obfReader,
                routingSection,
                dataLevel,
                resultOut);
        }
    }
    
    return true;
}

bool OsmAnd::ObfDataInterface::loadMapObjects(
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* outBinaryMapObjects,
    QList< std::shared_ptr<const OsmAnd::Road> >* outRoads,
    MapSurfaceType* outSurfaceType,
    const ZoomLevel zoom,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfMapSectionReader::FilterByIdFunction filterMapObjectsById /*= nullptr*/,
    ObfMapSectionReader::DataBlocksCache* binaryMapObjectsCache /*= nullptr*/,
    QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedBinaryMapObjectsCacheEntries /*= nullptr*/,
    const FilterRoadsByIdFunction filterRoadsById /*= nullptr*/,
    ObfRoutingSectionReader::DataBlocksCache* roadsCache /*= nullptr*/,
    QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedRoadsCacheEntries /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const binaryMapObjectsMetric /*= nullptr*/,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const roadsMetric /*= nullptr*/)
{
    auto mergedSurfaceType = MapSurfaceType::Undefined;
    std::shared_ptr<const ObfReader> basemapReader;

    QSet<QString> processedMapSectionsNames;

    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();

        // Handle basemap
        if (obfInfo->isBasemapWithCoastlines)
        {
            // In case there's more than 1 basemap reader present, use only first and warn about this fact
            if (basemapReader)
            {
                LogPrintf(LogSeverityLevel::Warning, "More than 1 basemap available");
                continue;
            }

            // Save basemap reader for later use
            basemapReader = obfReader;

            // In case requested zoom is more detailed than basemap max zoom, skip basemap processing for now
            if (zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
                continue;
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            // Remember that this section was processed (by name)
            processedMapSectionsNames.insert(mapSection->name);

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                obfReader,
                mapSection,
                zoom,
                bbox31,
                outBinaryMapObjects,
                &surfaceTypeToMerge,
                filterMapObjectsById,
                nullptr,
                binaryMapObjectsCache,
                outReferencedBinaryMapObjectsCacheEntries,
                queryController,
                binaryMapObjectsMetric);
            if (surfaceTypeToMerge != MapSurfaceType::Undefined)
            {
                if (mergedSurfaceType == MapSurfaceType::Undefined)
                    mergedSurfaceType = surfaceTypeToMerge;
                else if (mergedSurfaceType != surfaceTypeToMerge)
                    mergedSurfaceType = MapSurfaceType::Mixed;
            }
        }
    }

    // In case there's basemap available and requested zoom is more detailed than basemap max zoom level,
    // read tile from MaxBasemapZoomLevel that covers requested tile
    if (basemapReader && zoom > static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel))
    {
        const auto& obfInfo = basemapReader->obtainInfo();

        // Calculate proper bbox31 on MaxBasemapZoomLevel (if possible)
        const AreaI *pBasemapBBox31 = nullptr;
        AreaI basemapBBox31;
        if (bbox31)
        {
            pBasemapBBox31 = &basemapBBox31;
            basemapBBox31 = Utilities::roundBoundingBox31(
                *bbox31,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel));
        }

        for (const auto& mapSection : constOf(obfInfo->mapSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            // Read objects from each map section
            auto surfaceTypeToMerge = MapSurfaceType::Undefined;
            OsmAnd::ObfMapSectionReader::loadMapObjects(
                basemapReader,
                mapSection,
                static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel),
                pBasemapBBox31,
                outBinaryMapObjects,
                &surfaceTypeToMerge,
                filterMapObjectsById,
                nullptr,
                binaryMapObjectsCache,
                outReferencedBinaryMapObjectsCacheEntries,
                queryController,
                binaryMapObjectsMetric);

            // Basemap must always have a surface type defined
            assert(surfaceTypeToMerge != MapSurfaceType::Undefined);
            if (mergedSurfaceType == MapSurfaceType::Undefined)
                mergedSurfaceType = surfaceTypeToMerge;
            else if (mergedSurfaceType != surfaceTypeToMerge)
                mergedSurfaceType = MapSurfaceType::Mixed;
        }
    }

    // In case there was a basemap present, Undefined is Land
    if (mergedSurfaceType == MapSurfaceType::Undefined && !basemapReader)
        mergedSurfaceType = MapSurfaceType::FullLand;

    if (outSurfaceType)
        *outSurfaceType = mergedSurfaceType;

    if (zoom > ObfMapSectionLevel::MaxBasemapZoomLevel)
    {
        for (const auto& obfReader : constOf(obfReaders))
        {
            if (queryController && queryController->isAborted())
                return false;

            const auto& obfInfo = obfReader->obtainInfo();

            // Skip all OBF readers that have map section, since they were already processed
            if (!obfInfo->mapSections.isEmpty())
                continue;

            for (const auto& routingSection : constOf(obfInfo->routingSections))
            {
                // Check if request is aborted
                if (queryController && queryController->isAborted())
                    return false;

                // Check that map section with same name was not processed from other file
                if (processedMapSectionsNames.contains(routingSection->name))
                    continue;

                // Read objects from each map section
                OsmAnd::ObfRoutingSectionReader::loadRoads(
                    obfReader,
                    routingSection,
                    RoutingDataLevel::Detailed,
                    bbox31,
                    outRoads,
                    filterRoadsById,
                    nullptr,
                    roadsCache,
                    outReferencedRoadsCacheEntries,
                    queryController,
                    roadsMetric);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadAmenityCategories(
    QHash<QString, QStringList>* outCategories,
    const AreaI* const pBbox31 /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& poiSection : constOf(obfInfo->poiSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (pBbox31)
            {
                bool accept = false;
                accept = accept || poiSection->area31.contains(*pBbox31);
                accept = accept || poiSection->area31.intersects(*pBbox31);
                accept = accept || pBbox31->contains(poiSection->area31);

                if (!accept)
                    continue;
            }

            std::shared_ptr<const ObfPoiSectionCategories> categories;
            OsmAnd::ObfPoiSectionReader::loadCategories(
                obfReader,
                poiSection,
                categories,
                queryController);

            if (!categories)
                continue;

            for (auto mainCategoryIndex = 0; mainCategoryIndex < categories->mainCategories.size(); mainCategoryIndex++)
            {
                outCategories->insert(
                    categories->mainCategories[mainCategoryIndex],
                    categories->subCategories[mainCategoryIndex]);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadAmenities(
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const AreaI* const pBbox31 /*= nullptr*/,
    const TileAcceptorFunction tileFilter /*= nullptr*/,
    const ZoomLevel zoomFilter /*= InvalidZoomLevel*/,
    const QHash<QString, QStringList>* const categoriesFilter /*= nullptr*/,
    const ObfPoiSectionReader::VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& poiSection : constOf(obfInfo->poiSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (pBbox31)
            {
                bool accept = false;
                accept = accept || poiSection->area31.contains(*pBbox31);
                accept = accept || poiSection->area31.intersects(*pBbox31);
                accept = accept || pBbox31->contains(poiSection->area31);

                if (!accept)
                    continue;
            }

            QSet<ObfPoiCategoryId> categoriesFilterById;
            if (categoriesFilter)
            {
                std::shared_ptr<const ObfPoiSectionCategories> categories;
                OsmAnd::ObfPoiSectionReader::loadCategories(
                    obfReader,
                    poiSection,
                    categories,
                    queryController);

                if (!categories)
                    continue;

                for (const auto& categoriesFilterEntry : rangeOf(constOf(*categoriesFilter)))
                {
                    const auto mainCategoryIndex = categories->mainCategories.indexOf(categoriesFilterEntry.key());
                    if (mainCategoryIndex < 0)
                        continue;

                    const auto& subcategories = categories->subCategories[mainCategoryIndex];
                    if (categoriesFilterEntry.value().isEmpty())
                    {
                        for (auto subCategoryIndex = 0; subCategoryIndex < subcategories.size(); subCategoryIndex++)
                            categoriesFilterById.insert(ObfPoiCategoryId::create(mainCategoryIndex, subCategoryIndex));
                    }
                    else
                    {
                        for (const auto& subcategory : constOf(categoriesFilterEntry.value()))
                        {
                            const auto subCategoryIndex = subcategories.indexOf(subcategory);
                            if (subCategoryIndex < 0)
                                continue;

                            categoriesFilterById.insert(ObfPoiCategoryId::create(mainCategoryIndex, subCategoryIndex));
                        }
                    }
                }
            }

            OsmAnd::ObfPoiSectionReader::loadAmenities(
                obfReader,
                poiSection,
                outAmenities,
                pBbox31,
                tileFilter,
                zoomFilter,
                categoriesFilter ? &categoriesFilterById : nullptr,
                visitor,
                queryController);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::scanAmenitiesByName(
    const QString& query,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const PointI* const xy31 /*= nullptr*/,
    const AreaI* const pBbox31 /*= nullptr*/,
    const TileAcceptorFunction tileFilter /*= nullptr*/,
    const QHash<QString, QStringList>* const categoriesFilter /*= nullptr*/,
    const ObfPoiSectionReader::VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    typedef std::pair< std::shared_ptr<const ObfReader>, Ref<ObfPoiSectionInfo> > OrderedSection;
    std::vector< OrderedSection > orderedSections;
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto &obfInfo = obfReader->obtainInfo();
        for (const auto& poiSection : constOf(obfInfo->poiSections))
        {
            if (queryController && queryController->isAborted())
            {
                return false;
            }


            if (pBbox31)
            {
                bool accept = false;
                accept = accept || poiSection->area31.contains(*pBbox31);
                accept = accept || poiSection->area31.intersects(*pBbox31);
                accept = accept || pBbox31->contains(poiSection->area31);

                if (!accept)
                {
                    continue;
                }
            }

            orderedSections.push_back(OrderedSection(obfReader, poiSection));
        }
    }

    if (pBbox31)
    {
        const auto bboxCenter = pBbox31->center();

        // Sort blocks by data offset to force forward-only seeking
        std::sort(
            orderedSections,
            [bboxCenter]
            (const OrderedSection& l, const OrderedSection& r) -> bool
            {
                const auto lCenter = l.second->area31.center();
                const auto lSqDistance = (lCenter - bboxCenter).squareNorm();

                const auto rCenter = r.second->area31.center();
                const auto rSqDistance = (rCenter - bboxCenter).squareNorm();

                return lSqDistance < rSqDistance;
            });
    }

    for (const auto& orderedSection : constOf(orderedSections))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfReader = orderedSection.first;
        const auto& poiSection = orderedSection.second;

        QSet<ObfPoiCategoryId> categoriesFilterById;
        if (categoriesFilter)
        {
            std::shared_ptr<const ObfPoiSectionCategories> categories;
            OsmAnd::ObfPoiSectionReader::loadCategories(
                obfReader,
                poiSection,
                categories,
                queryController);

            if (!categories)
                continue;

            for (const auto& categoriesFilterEntry : rangeOf(constOf(*categoriesFilter)))
            {
                const auto mainCategoryIndex = categories->mainCategories.indexOf(categoriesFilterEntry.key());
                if (mainCategoryIndex < 0)
                    continue;

                const auto& subcategories = categories->subCategories[mainCategoryIndex];
                if (categoriesFilterEntry.value().isEmpty())
                {
                    for (auto subCategoryIndex = 0; subCategoryIndex < subcategories.size(); subCategoryIndex++)
                        categoriesFilterById.insert(ObfPoiCategoryId::create(mainCategoryIndex, subCategoryIndex));
                }
                else
                {
                    for (const auto& subcategory : constOf(categoriesFilterEntry.value()))
                    {
                        const auto subCategoryIndex = subcategories.indexOf(subcategory);
                        if (subCategoryIndex < 0)
                            continue;

                        categoriesFilterById.insert(ObfPoiCategoryId::create(mainCategoryIndex, subCategoryIndex));
                    }
                }
            }
        }

        OsmAnd::ObfPoiSectionReader::scanAmenitiesByName(
            obfReader,
            poiSection,
            query,
            outAmenities,
            xy31,
            pBbox31,
            tileFilter,
            categoriesFilter ? &categoriesFilterById : nullptr,
            visitor,
            queryController);
    }

    return true;
}

bool OsmAnd::ObfDataInterface::findAmenityByObfMapObject(
    const std::shared_ptr<const OsmAnd::ObfMapObject>& obfMapObject,
    std::shared_ptr<const OsmAnd::Amenity>* const outAmenity,
    const AreaI* const pBbox31 /*= nullptr*/,
    const TileAcceptorFunction tileFilter /*= nullptr*/,
    const ZoomLevel zoomFilter /*= InvalidZoomLevel*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    std::shared_ptr<const OsmAnd::Amenity> foundAmenityById;
    std::shared_ptr<const OsmAnd::Amenity> foundAmenityByName;

    uint64_t obfId = obfMapObject->id.id >> 7;
    const auto visitor =
        [obfId, &foundAmenityById, &foundAmenityByName, obfMapObject]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            if (foundAmenityById)
                return false;
            
            // todo: wrong IF since mapObject->id != amenity->id
            if (amenity->id >> 1 == obfId)
                foundAmenityById = amenity;
            else
                for (const auto& caption : obfMapObject->captions.values())
                {
                    if (amenity->nativeName == caption || amenity->localizedNames.values().contains(caption)) {
                        foundAmenityByName = amenity;
                        break;
                    }
                }

            return false;
        };

    const auto subQueryController = std::make_shared<FunctorQueryController>(
        [&foundAmenityById]
        (const FunctorQueryController* const queryController) -> bool
        {
            return static_cast<bool>(foundAmenityById);
        });

    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& poiSection : constOf(obfInfo->poiSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (pBbox31)
            {
                bool accept = false;
                accept = accept || poiSection->area31.contains(*pBbox31);
                accept = accept || poiSection->area31.intersects(*pBbox31);
                accept = accept || pBbox31->contains(poiSection->area31);

                if (!accept)
                    continue;
            }

            OsmAnd::ObfPoiSectionReader::loadAmenities(
                obfReader,
                poiSection,
                nullptr,
                pBbox31,
                tileFilter,
                zoomFilter,
                nullptr,
                visitor,
                subQueryController);

            if (foundAmenityById)
            {
                if (outAmenity)
                    *outAmenity = foundAmenityById;
                return true;
            }
            else if (foundAmenityByName)
            {
                if (outAmenity)
                    *outAmenity = foundAmenityByName;
                return true;
            }
        }
    }

    return false;
}

bool OsmAnd::ObfDataInterface::findAmenityForObfMapObject(
    const std::shared_ptr<const OsmAnd::ObfMapObject>& obfMapObject,
    std::shared_ptr<const OsmAnd::Amenity>* const outAmenity,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    const auto alignMask = (1u << 7) - 1;
    auto alignedBBox = obfMapObject->bbox31;
    alignedBBox.top() &= ~alignMask;
    alignedBBox.left() &= ~alignMask;
    alignedBBox.bottom() |= alignMask;
    alignedBBox.right() |= alignMask;

    return findAmenityByObfMapObject(
        obfMapObject,
        outAmenity,
        &alignedBBox,
        nullptr,
        InvalidZoomLevel,
        queryController);
}

bool OsmAnd::ObfDataInterface::scanAddressesByName(
    const QString& query,
    const StringMatcherMode matcherMode,
    QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter /*= fullObfAddressStreetGroupTypesMask()*/,
    const bool includeStreets /*= true*/,
    const bool strictMatch /*= false*/,
    const ObfAddressSectionReader::VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (bbox31)
            {
                bool accept = false;
                accept = accept || addressSection->area31.contains(*bbox31);
                accept = accept || addressSection->area31.intersects(*bbox31);
                accept = accept || bbox31->contains(addressSection->area31);

                if (!accept)
                    continue;
            }

            OsmAnd::ObfAddressSectionReader::scanAddressesByName(
                obfReader,
                addressSection,
                query,
                matcherMode,
                outAddresses,
                bbox31,
                streetGroupTypesFilter,
                includeStreets,
                strictMatch,
                visitor,
                queryController);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadStreetGroups(
    QList< std::shared_ptr<const StreetGroup> >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter /*= fullObfAddressStreetGroupTypesMask()*/,
    const ObfAddressSectionReader::StreetGroupVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (bbox31)
            {
                bool accept = false;
                accept = accept || addressSection->area31.contains(*bbox31);
                accept = accept || addressSection->area31.intersects(*bbox31);
                accept = accept || bbox31->contains(addressSection->area31);

                if (!accept)
                    continue;
            }

            OsmAnd::ObfAddressSectionReader::loadStreetGroups(
                obfReader,
                addressSection,
                resultOut,
                bbox31,
                streetGroupTypesFilter,
                visitor,
                queryController);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::preloadStreets(
    const QList< std::shared_ptr<StreetGroup> >& streetGroups,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;
        
        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;
            
            for (const auto& streetGroup : constOf(streetGroups))
            {
                if (addressSection != streetGroup->obfSection)
                    continue;
                
                QList< std::shared_ptr<const Street> > intermediateResult;
                OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
                                                                      obfReader,
                                                                      streetGroup,
                                                                      &intermediateResult,
                                                                      nullptr,
                                                                      nullptr,
                                                                      queryController);
                
                streetGroup->streets = qMove(intermediateResult);
            }
        }
    }
    
    return true;
}

bool OsmAnd::ObfDataInterface::loadStreetsFromGroups(
    const QList< std::shared_ptr<const StreetGroup> >& streetGroups,
    QHash< std::shared_ptr<const StreetGroup>, QList< std::shared_ptr<const Street> > >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressSectionReader::StreetVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (bbox31)
            {
                bool accept = false;
                accept = accept || addressSection->area31.contains(*bbox31);
                accept = accept || addressSection->area31.intersects(*bbox31);
                accept = accept || bbox31->contains(addressSection->area31);

                if (!accept)
                    continue;
            }

            for (const auto& streetGroup : constOf(streetGroups))
            {
                if (addressSection != streetGroup->obfSection)
                    continue;

                QList< std::shared_ptr<const Street> > intermediateResult;
                OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
                    obfReader,
                    streetGroup,
                    resultOut ? &intermediateResult : nullptr,
                    bbox31,
                    visitor,
                    queryController);

                if (resultOut)
                    resultOut->insert(streetGroup, intermediateResult);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::preloadBuildings(
    const QList< std::shared_ptr<Street> >& streets,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;
        
        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;
            
            for (const auto& street : constOf(streets))
            {
                if (addressSection != street->streetGroup->obfSection)
                    continue;
                
                QList< std::shared_ptr<const Building> > intermediateResult;
                OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
                                                                         obfReader,
                                                                         street,
                                                                         &intermediateResult,
                                                                         nullptr,
                                                                         nullptr,
                                                                         queryController);

                street->buildings = qMove(intermediateResult);
            }
        }
    }
    
    return true;
}

bool OsmAnd::ObfDataInterface::loadBuildingsFromStreets(
    const QList< std::shared_ptr<const Street> >& streets,
    QHash< std::shared_ptr<const Street>, QList< std::shared_ptr<const Building> > >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressSectionReader::BuildingVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (bbox31)
            {
                bool accept = false;
                accept = accept || addressSection->area31.contains(*bbox31);
                accept = accept || addressSection->area31.intersects(*bbox31);
                accept = accept || bbox31->contains(addressSection->area31);

                if (!accept)
                    continue;
            }

            for (const auto& street : constOf(streets))
            {
                if (addressSection != street->streetGroup->obfSection)
                    continue;

                QList< std::shared_ptr<const Building> > intermediateResult;
                OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
                    obfReader,
                    street,
                    resultOut ? &intermediateResult : nullptr,
                    bbox31,
                    visitor,
                    queryController);

                if (resultOut)
                    resultOut->insert(street, intermediateResult);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadIntersectionsFromStreets(
    const QList< std::shared_ptr<const Street> >& streets,
    QHash< std::shared_ptr<const Street>, QList< std::shared_ptr<const StreetIntersection> > >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressSectionReader::IntersectionVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;

        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& addressSection : constOf(obfInfo->addressSections))
        {
            if (queryController && queryController->isAborted())
                return false;

            if (bbox31)
            {
                bool accept = false;
                accept = accept || addressSection->area31.contains(*bbox31);
                accept = accept || addressSection->area31.intersects(*bbox31);
                accept = accept || bbox31->contains(addressSection->area31);

                if (!accept)
                    continue;
            }

            for (const auto& street : constOf(streets))
            {
                if (addressSection != street->streetGroup->obfSection)
                    continue;

                QList< std::shared_ptr<const StreetIntersection> > intermediateResult;
                OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(
                    obfReader,
                    street,
                    resultOut ? &intermediateResult : nullptr,
                    bbox31,
                    visitor,
                    queryController);

                if (resultOut)
                    resultOut->insert(street, intermediateResult);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::searchTransportIndex(
    QList< std::shared_ptr<const TransportStop> >* outTransportStops /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    ObfSectionInfo::StringTable* const stringTable /*= nullptr*/,
    const ObfTransportSectionReader::TransportStopVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        if (queryController && queryController->isAborted())
            return false;
        
        const auto& obfInfo = obfReader->obtainInfo();
        for (const auto& transportSection : constOf(obfInfo->transportSections))
        {
            if (transportSection->stopsLength == 0)
                return false;

            if (queryController && queryController->isAborted())
                return false;
            
            if (bbox31)
            {
                bool accept = false;
                accept = accept || transportSection->area31.contains(*bbox31);
                accept = accept || transportSection->area31.intersects(*bbox31);
                accept = accept || bbox31->contains(transportSection->area31);
                
                if (!accept)
                    continue;
            }
            
            OsmAnd::ObfTransportSectionReader::searchTransportStops(
                                                                    obfReader,
                                                                    transportSection,
                                                                    outTransportStops,
                                                                    bbox31,
                                                                    stringTable,
                                                                    visitor,
                                                                    queryController);
        }
    }
    
    return true;
}

const std::shared_ptr<const OsmAnd::ObfTransportSectionInfo> OsmAnd::ObfDataInterface::getTransportSectionInfo(
    const QList<Ref<ObfTransportSectionInfo>>& sections,
    const uint32_t filePointer)
{
    for (const auto& transportSection : constOf(sections))
        if (transportSection->offset <= filePointer && (filePointer - transportSection->offset) < transportSection->length)
            return transportSection.shared_ptr();

    return nullptr;
}

bool OsmAnd::ObfDataInterface::getTransportRoutes(
    const std::shared_ptr<const TransportStop>& transportStop,
    QList< std::shared_ptr<const TransportRoute> >* resultOut /*= nullptr*/,
    ObfSectionInfo::StringTable* const stringTable /*= nullptr*/,
    const ObfTransportSectionReader::TransportRouteVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    QHash<uint32_t, std::shared_ptr<TransportRoute>> result;
    QHash<const int, QList<uint32_t>> groupPoints;
    QHash<const int, std::shared_ptr<const ObfReader>> readers;
    QHash<const int, std::shared_ptr<const ObfTransportSectionInfo>> sections;
    for (auto filePointer : transportStop->referencesToRoutes)
    {
        for (const auto& obfReader : constOf(obfReaders))
        {
            if (queryController && queryController->isAborted())
                return false;
            
            const auto& obfInfo = obfReader->obtainInfo();
            const auto section = getTransportSectionInfo(constOf(obfInfo->transportSections), filePointer);
            if (section)
            {
                if (!groupPoints.contains(section->runtimeGeneratedId))
                {
                    if (!readers.contains(section->runtimeGeneratedId))
                        readers[section->runtimeGeneratedId] = obfReader;
                    if (!sections.contains(section->runtimeGeneratedId))
                        sections[section->runtimeGeneratedId] = section;
                    
                    groupPoints[section->runtimeGeneratedId] = QList<uint32_t>();
                }
                
                groupPoints[section->runtimeGeneratedId].push_back(filePointer);
            }
        }
    }
    
    for(const auto& entry : rangeOf(constOf(groupPoints)))
    {
        const auto sectionId = entry.key();
        auto reader = readers[sectionId];
        auto section = sections[sectionId];
        auto pointers = entry.value();
        qSort(pointers);
        auto stringTable = std::make_shared<ObfSectionInfo::StringTable>();
        for (const auto& filePointer : pointers)
        {
            auto transportRoute = OsmAnd::ObfTransportSectionReader::getTransportRoute(reader, section, filePointer, stringTable.get(), false);
            result[filePointer] = transportRoute;
        }
        OsmAnd::ObfTransportSectionReader::initializeStringTable(reader, section, stringTable.get());
        for (auto r : result.values())
            OsmAnd::ObfTransportSectionReader::initializeNames(false, stringTable.get(), r);
    }
    
    for (const auto& transportRoute : result.values())
    {
        if (!visitor || visitor(transportRoute))
        {
            if (resultOut)
                resultOut->push_back(transportRoute);
        }
    }
    return true;
}

bool OsmAnd::ObfDataInterface::transportStopBelongsTo(const std::shared_ptr<const TransportStop>& s)
{
    for (const auto& obfReader : constOf(obfReaders))
    {
        const auto& obfInfo = obfReader->obtainInfo();
        const auto section = getTransportSectionInfo(constOf(obfInfo->transportSections), s->offset);
        if (section)
            return true;
    }
    return false;
}
