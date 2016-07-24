#include "ObfDataInterface.h"

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QSet>
#include "restore_internal_warnings.h"

#include "Amenity.h"
#include "FunctorQueryController.h"
#include "IQueryController.h"
#include "ObfAddressSectionInfo.h"
#include "ObfAddressSectionReader.h"
#include "ObfBuilding.h"
#include "ObfInfo.h"
#include "ObfMapObject.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionReader.h"
#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionReader.h"
#include "ObfReader.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionReader.h"
#include "ObfStreetGroup.h"
#include "ObfStreet.h"
#include "QKeyValueIterator.h"
#include "Ref.h"
#include "Street.h"
#include "StreetGroup.h"


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
            pBbox31,
            tileFilter,
            categoriesFilter ? &categoriesFilterById : nullptr,
            visitor,
            queryController);
    }

    return true;
}

bool OsmAnd::ObfDataInterface::findAmenityById(
    const ObfObjectId id,
    std::shared_ptr<const OsmAnd::Amenity>* const outAmenity,
    const AreaI* const pBbox31 /*= nullptr*/,
    const TileAcceptorFunction tileFilter /*= nullptr*/,
    const ZoomLevel zoomFilter /*= InvalidZoomLevel*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    std::shared_ptr<const OsmAnd::Amenity> foundAmenity;

    const auto visitor =
        [id, &foundAmenity]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            // todo: wrong IF since mapObject->id != amenity->id
            //if (amenity->id == id)
                foundAmenity = amenity;

            return false;
        };

    const auto subQueryController = std::make_shared<FunctorQueryController>(
        [&foundAmenity]
        (const FunctorQueryController* const queryController) -> bool
        {
            return static_cast<bool>(foundAmenity);
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

            if (foundAmenity)
            {
                if (outAmenity)
                    *outAmenity = foundAmenity;
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

    return findAmenityById(
        obfMapObject->id,
        outAmenity,
        &alignedBBox,
        nullptr,
        InvalidZoomLevel,
        queryController);
}

bool OsmAnd::ObfDataInterface::scanAddressesByName(
    const ObfAddressSectionReader::Filter& filter,
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

            if (filter.matches(addressSection->area31))
                OsmAnd::ObfAddressSectionReader::scanAddressesByName(
                            obfReader,
                            addressSection,
                            filter,
                            queryController);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadStreetGroups(
    const Filter& filter,
    QList< std::shared_ptr<const ObfStreetGroup> >* resultOut /*= nullptr*/,
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

            if (!filter.matches(addressSection->area31))
                continue;

            OsmAnd::ObfAddressSectionReader::loadStreetGroups(
                obfReader,
                addressSection,
                filter,
                queryController);
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadStreetsFromGroups(
        const QVector<std::shared_ptr<const ObfStreetGroup>>& streetGroups,
        const Filter& filter,
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

            if (!filter.matches(addressSection->area31))
                continue;

            for (const auto& streetGroup : constOf(streetGroups))
            {
                if (addressSection != streetGroup->obfSection())
                    continue;

                OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
                    obfReader,
                    streetGroup,
                    filter,
                    queryController);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadBuildingsFromStreets(
        const QVector<std::shared_ptr<const ObfStreet>>& streets,
        const ObfAddressSectionReader::Filter& filter,
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

            if (!filter.matches(addressSection->area31))
                continue;

            for (const auto& street : constOf(streets))
            {
                if (addressSection != street->obfStreetGroup()->obfSection())
                    continue;

                OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
                    obfReader,
                    street,
                    filter,
                    queryController);
            }
        }
    }

    return true;
}

bool OsmAnd::ObfDataInterface::loadIntersectionsFromStreets(
    const QVector<std::shared_ptr<const ObfStreet>>& streets,
    const Filter& filter,
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

            if (!filter.matches(addressSection->area31))
                continue;

            for (const auto& street : constOf(streets))
            {
                if (addressSection != street->obfStreetGroup()->obfSection())
                    continue;

                OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(
                    obfReader,
                    street,
                    filter,
                    queryController);
            }
        }
    }

    return true;
}
