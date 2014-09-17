#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/SharedResourcesContainer.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfRoutingSectionInfo;
    namespace Model
    {
        class Road;
    }
    class IQueryController;
    namespace ObfRoutingSectionReader_Metrics
    {
        struct Metric_loadRoads;
    }

    class ObfRoutingSectionReader_P;
    class OSMAND_CORE_API ObfRoutingSectionReader
    {
    public:
        typedef std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> VisitorFunction;
        typedef ObfRoutingSectionDataBlockId DataBlockId;

        class OSMAND_CORE_API DataBlock Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(DataBlock);
        private:
        protected:
            DataBlock(
                const DataBlockId id,
                const RoutingDataLevel dataLevel,
                const AreaI area31,
                const QList< std::shared_ptr<const OsmAnd::Model::Road> >& roads);
        public:
            ~DataBlock();

            const DataBlockId id;
            const RoutingDataLevel dataLevel;
            const AreaI area31;
            const QList< std::shared_ptr<const OsmAnd::Model::Road> > roads;

        friend class OsmAnd::ObfRoutingSectionReader;
        friend class OsmAnd::ObfRoutingSectionReader_P;
        };

        class OSMAND_CORE_API DataBlocksCache : public SharedResourcesContainer < DataBlockId, const DataBlock >
        {
        public:
            typedef ObfRoutingSectionReader::DataBlockId DataBlockId;

        private:
        protected:
        public:
            DataBlocksCache();
            virtual ~DataBlocksCache();

            virtual bool shouldCacheBlock(
                const DataBlockId id,
                const RoutingDataLevel dataLevel,
                const AreaI blockBBox31,
                const AreaI* const queryArea31 = nullptr) const;
        };

    private:
        ObfRoutingSectionReader();
        ~ObfRoutingSectionReader();
    protected:
    public:
        static void loadRoads(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const RoutingDataLevel dataLevel,
            const AreaI* const bbox31 = nullptr,
            QList< std::shared_ptr<const OsmAnd::Model::Road> >* resultOut = nullptr,
            const FilterRoadsByIdFunction filterById = nullptr,
            const VisitorFunction visitor = nullptr,
            DataBlocksCache* cache = nullptr,
            QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries = nullptr,
            const IQueryController* const controller = nullptr,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_READER_H_)
