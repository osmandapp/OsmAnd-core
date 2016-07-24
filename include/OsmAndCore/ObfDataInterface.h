#ifndef _OSMAND_CORE_OBF_DATA_INTERFACE_H_
#define _OSMAND_CORE_OBF_DATA_INTERFACE_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfAddressSectionReader.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>
#include <OsmAndCore/Data/ObfPoiSectionReader.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    class IQueryController;
    class ObfBuilding;
    class ObfFile;
    class ObfMapObject;
    class ObfReader;
    class ObfStreet;
    class ObfStreetGroup;

    class OSMAND_CORE_API ObfDataInterface
    {
        Q_DISABLE_COPY_AND_MOVE(ObfDataInterface)

    public:
        using Filter = ObfAddressSectionReader::Filter;

        ObfDataInterface(const QList< std::shared_ptr<const ObfReader> >& obfReaders);
        virtual ~ObfDataInterface();

        const QList< std::shared_ptr<const ObfReader> > obfReaders;

        bool loadObfFiles(
            QList< std::shared_ptr<const ObfFile> >* outFiles = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool loadBinaryMapObjects(
            QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
            MapSurfaceType* outSurfaceType,
            const ZoomLevel zoom,
            const AreaI* const bbox31 = nullptr,
            const ObfMapSectionReader::FilterByIdFunction filterById = nullptr,
            ObfMapSectionReader::DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedCacheEntries = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric = nullptr);

        bool loadRoads(
            const RoutingDataLevel dataLevel,
            const AreaI* const bbox31 = nullptr,
            QList< std::shared_ptr<const OsmAnd::Road> >* resultOut = nullptr,
            const FilterRoadsByIdFunction filterById = nullptr,
            const ObfRoutingSectionReader::VisitorFunction visitor = nullptr,
            ObfRoutingSectionReader::DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedCacheEntries = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric = nullptr);

        bool loadMapObjects(
            QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* outBinaryMapObjects,
            QList< std::shared_ptr<const OsmAnd::Road> >* outRoads,
            MapSurfaceType* outSurfaceType,
            const ZoomLevel zoom,
            const AreaI* const bbox31 = nullptr,
            const ObfMapSectionReader::FilterByIdFunction filterMapObjectsById = nullptr,
            ObfMapSectionReader::DataBlocksCache* binaryMapObjectsCache = nullptr,
            QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedBinaryMapObjectsCacheEntries = nullptr,
            const FilterRoadsByIdFunction filterRoadsById = nullptr,
            ObfRoutingSectionReader::DataBlocksCache* roadsCache = nullptr,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedRoadsCacheEntries = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const binaryMapObjectsMetric = nullptr,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const roadsMetric = nullptr);

        bool loadAmenityCategories(
            QHash<QString, QStringList>* outCategories,
            const AreaI* const pBbox31 = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool loadAmenities(
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const AreaI* const bbox31 = nullptr,
            const TileAcceptorFunction tileFilter = nullptr,
            const ZoomLevel zoomFilter = InvalidZoomLevel,
            const QHash<QString, QStringList>* const categoriesFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool scanAmenitiesByName(
            const QString& query,
            QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
            const AreaI* const bbox31 = nullptr,
            const TileAcceptorFunction tileFilter = nullptr,
            const QHash<QString, QStringList>* const categoriesFilter = nullptr,
            const ObfPoiSectionReader::VisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool findAmenityById(
            const ObfObjectId id,
            std::shared_ptr<const OsmAnd::Amenity>* const outAmenity,
            const AreaI* const bbox31 = nullptr,
            const TileAcceptorFunction tileFilter = nullptr,
            const ZoomLevel zoomFilter = InvalidZoomLevel,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool findAmenityForObfMapObject(
            const std::shared_ptr<const OsmAnd::ObfMapObject>& obfMapObject,
            std::shared_ptr<const OsmAnd::Amenity>* const outAmenity,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool scanAddressesByName(
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool loadStreetGroups(
                const Filter& filter = Filter{},
                QList< std::shared_ptr<const ObfStreetGroup> >* resultOut = nullptr,
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool loadStreetsFromGroups(const QVector<std::shared_ptr<const ObfStreetGroup> > &streetGroups,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool loadBuildingsFromStreets(const QVector<std::shared_ptr<const ObfStreet> > &streets,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        bool loadIntersectionsFromStreets(const QVector<std::shared_ptr<const ObfStreet> > &streets,
                const Filter& filter = Filter{},
                const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_DATA_INTERFACE_H_)
