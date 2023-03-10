#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/SharedByZoomResourcesContainer.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfMapSectionInfo;
    class BinaryMapObject;
    class IQueryController;
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects;
    }

    class ObfMapSectionReader_P;
    class OSMAND_CORE_API ObfMapSectionReader
    {
    public:
        typedef ObfMapSectionDataBlockId DataBlockId;
        typedef std::function < bool(
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const DataBlockId& blockId,
            const ObfObjectId mapObjectId,
            const AreaI& bbox,
            const ZoomLevel firstZoomLevel,
            const ZoomLevel lastZoomLevel,
            const ZoomLevel requestedZoomLevel) > FilterByIdFunction;
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::BinaryMapObject>&)> VisitorFunction;

        class OSMAND_CORE_API DataBlock Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(DataBlock);
        private:
        protected:
            DataBlock(
                const DataBlockId id,
                const AreaI bbox31,
                const MapSurfaceType surfaceType,
                const QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >& mapObjects);
        public:
            ~DataBlock();

            const DataBlockId id;
            const AreaI bbox31;
            const MapSurfaceType surfaceType;
            const QList< std::shared_ptr<const OsmAnd::BinaryMapObject> > mapObjects;

        friend class OsmAnd::ObfMapSectionReader;
        friend class OsmAnd::ObfMapSectionReader_P;
        };

        class OSMAND_CORE_API DataBlocksCache : public SharedByZoomResourcesContainer < DataBlockId, const DataBlock >
        {
        public:
            typedef ObfMapSectionReader::DataBlockId DataBlockId;

        private:
        protected:
        public:
            DataBlocksCache();
            virtual ~DataBlocksCache();

            virtual bool shouldCacheBlock(const DataBlockId id, const AreaI blockBBox31, const AreaI* const queryArea31 = nullptr) const;
        };

    private:
        ObfMapSectionReader();
        ~ObfMapSectionReader();
    protected:
    public:
        static void loadMapObjects(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const ZoomLevel zoom,
            const AreaI* const bbox31 = nullptr,
            QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut = nullptr,
            MapSurfaceType* outBBoxOrSectionSurfaceType = nullptr,
            const FilterByIdFunction filterById = nullptr,
            const VisitorFunction visitor = nullptr,
            DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric = nullptr,
            bool coastlineOnly = false);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_H_)
