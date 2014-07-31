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
#include <OsmAndCore/Data/DataTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfMapSectionInfo;
    namespace Model
    {
        class BinaryMapObject;
    }
    class IQueryController;
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects;
    }

    class ObfMapSectionReader_P;
    class OSMAND_CORE_API ObfMapSectionReader
    {
    public:
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Model::BinaryMapObject>&)> VisitorFunction;
        typedef ObfMapSectionDataBlockId DataBlockId;

        class OSMAND_CORE_API DataBlock Q_DECL_FINAL
        {
        private:
        protected:
            DataBlock(
                const DataBlockId id,
                const AreaI bbox31,
                const MapFoundationType foundationType,
                const QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >& mapObjects);
        public:
            ~DataBlock();

            const DataBlockId id;
            const AreaI bbox31;
            const MapFoundationType foundationType;
            const QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> > mapObjects;

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
            QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >* resultOut = nullptr,
            MapFoundationType* foundationOut = nullptr,
            const FilterMapObjectsByIdFunction filterById = nullptr,
            const VisitorFunction visitor = nullptr,
            DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries = nullptr,
            const IQueryController* const controller = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_H_)
