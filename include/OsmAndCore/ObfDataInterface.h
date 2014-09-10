#ifndef _OSMAND_CORE_OBF_DATA_INTERFACE_H_
#define _OSMAND_CORE_OBF_DATA_INTERFACE_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfFile;
    class ObfMapSectionInfo;
    namespace Model
    {
        class BinaryMapObject;
        class Road;
    }
    class IQueryController;
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects;
    }

    class OSMAND_CORE_API ObfDataInterface
    {
        Q_DISABLE_COPY_AND_MOVE(ObfDataInterface);
    private:
    protected:
    public:
        ObfDataInterface(const QList< std::shared_ptr<const ObfReader> >& obfReaders);
        virtual ~ObfDataInterface();

        const QList< std::shared_ptr<const ObfReader> > obfReaders;

        bool loadObfFiles(
            QList< std::shared_ptr<const ObfFile> >* outFiles = nullptr,
            const IQueryController* const controller = nullptr);

        bool loadBasemapPresenceFlag(
            bool& outBasemapPresent,
            const IQueryController* const controller = nullptr);

        bool loadMapObjects(
            QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >* resultOut,
            MapFoundationType* outFoundation,
            const ZoomLevel zoom,
            const AreaI* const bbox31 = nullptr,
            const FilterMapObjectsByIdFunction filterById = nullptr,
            ObfMapSectionReader::DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const ObfMapSectionReader::DataBlock> >* outReferencedCacheEntries = nullptr,
            const IQueryController* const controller = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric = nullptr);

        bool loadRoads(
            const RoutingDataLevel dataLevel,
            const AreaI* const bbox31 = nullptr,
            QList< std::shared_ptr<const OsmAnd::Model::Road> >* resultOut = nullptr,
            const FilterRoadsByIdFunction filterById = nullptr,
            const ObfRoutingSectionReader::VisitorFunction visitor = nullptr,
            ObfRoutingSectionReader::DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const ObfRoutingSectionReader::DataBlock> >* outReferencedCacheEntries = nullptr,
            const IQueryController* const controller = nullptr,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_DATA_INTERFACE_H_)
