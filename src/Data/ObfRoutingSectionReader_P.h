#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionReader.h"

namespace OsmAnd
{
    class ObfReader_P;
    struct ObfRoutingSectionDecodingRules;
    class ObfRoutingSectionLevel;
    class ObfRoutingSectionLevelTreeNode;
    class Road;
    class IQueryController;
    namespace ObfRoutingSectionReader_Metrics
    {
        struct Metric_loadRoads;
    }

    class ObfRoutingSectionReader;
    class ObfRoutingSectionReader_P Q_DECL_FINAL
    {
    public:
        typedef ObfRoutingSectionReader::VisitorFunction VisitorFunction;
        typedef ObfRoutingSectionReader::DataBlockId DataBlockId;
        typedef ObfRoutingSectionReader::DataBlock DataBlock;
        typedef ObfRoutingSectionReader::DataBlocksCache DataBlocksCache;

    private:
        ObfRoutingSectionReader_P();
        ~ObfRoutingSectionReader_P();

    protected:
        enum : uint32_t {
            ShiftCoordinates = 4,
        };

        static void read(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfRoutingSectionInfo>& section);

        static void readLevelTreeNodeBbox31(
            const ObfReader_P& reader,
            AreaI& outBbox31);

        static void readAttributeMapping(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<ObfRoutingSectionAttributeMapping>& attributeMapping);

        static void readAttributeMappingEntry(
            const ObfReader_P& reader,
            const uint32_t naturalId,
            const std::shared_ptr<ObfRoutingSectionAttributeMapping>& attributeMapping);

        static void readLevelTreeNodes(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<ObfRoutingSectionLevel>& level);

        static void readLevelTreeNode(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfRoutingSectionLevelTreeNode>& node,
            const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& parentNode);

        static void readLevelTreeNodeChildren(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& treeNode,
            QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> >* outNodesWithData,
            const AreaI* bbox31,
            const std::shared_ptr<const IQueryController>& queryController,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric);

        static void readRoadsBlock(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& tree,
            const DataBlockId& blockId,
            QList< std::shared_ptr<const OsmAnd::Road> >* resultOut,
            const AreaI* bbox31,
            const FilterRoadsByIdFunction filterById,
            const VisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric);

        static void readRoadsBlockIdsTable(
            const ObfReader_P& reader,
            QList<uint64_t>& ids);

        static void readRoadsBlockRestrictions(
            const ObfReader_P& reader,
            const QHash< uint32_t, std::shared_ptr<Road> >& roadsByInternalIds,
            const QList<uint64_t>& roadsInternalIdToGlobalIdMap);

        static void readRoad(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& treeNode,
            const AreaI* bbox31,
            const FilterRoadsByIdFunction filterById,
            const QList<uint64_t>& idsTable,
            uint32_t& internalId,
            std::shared_ptr<Road>& road,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric);

    public:
        static void loadRoads(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const RoutingDataLevel dataLevel,
            const AreaI* const bbox31,
            QList< std::shared_ptr<const OsmAnd::Road> >* resultOut,
            const FilterRoadsByIdFunction filterById,
            const VisitorFunction visitor,
            DataBlocksCache* cache,
            QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries,
            const std::shared_ptr<const IQueryController>& queryController,
            ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric);

        static void loadTreeNodes(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const RoutingDataLevel dataLevel,
            QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> >* resultOut);
        
    friend class OsmAnd::ObfRoutingSectionReader;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_READER_P_H_)
