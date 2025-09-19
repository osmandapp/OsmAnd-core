#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QHash>
#include <QMap>
#include <QSet>
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapCommonTypes.h"
#include "ObfMapSectionReader.h"
#include "MapStyleEvaluator.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfMapSectionInfo;
    class ObfMapSectionLevel;
    class ObfMapSectionAttributeMapping;
    class ObfMapSectionLevelTreeNode;
    class BinaryMapObject;
    class IQueryController;
    class MapPresentationEnvironment;
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects;
    }

    class ObfMapSectionReader;
    class ObfMapSectionReader_P Q_DECL_FINAL
    {
    public:
        typedef ObfMapSectionReader::FilterByIdFunction FilterByIdFunction;
        typedef ObfMapSectionReader::VisitorFunction VisitorFunction;
        typedef ObfMapSectionReader::DataBlockId DataBlockId;
        typedef ObfMapSectionReader::DataBlock DataBlock;
        typedef ObfMapSectionReader::DataBlocksCache DataBlocksCache;

    private:
        ObfMapSectionReader_P();
        ~ObfMapSectionReader_P();
        static bool isCoastline(const std::shared_ptr<const BinaryMapObject> & mObj);
        static QList< std::shared_ptr<const BinaryMapObject>> filterCoastline(QList< std::shared_ptr<const BinaryMapObject>> & list);

    protected:
        static void read(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfMapSectionInfo>& section);

        static void readMapLevelHeader(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfMapSectionLevel>& level);

        static void readAttributeMapping(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<ObfMapSectionAttributeMapping>& attributeMapping);

        static void readAttributeMappingEntry(
            const ObfReader_P& reader,
            const uint32_t naturalId,
            const std::shared_ptr<ObfMapSectionAttributeMapping>& attributeMapping);

        static void readMapLevelTreeNodes(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const ObfMapSectionLevel>& level,
            QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >& nodes);

        static void readTreeNode(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const AreaI& parentArea,
            const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode);

        static void readTreeNodeChildren(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const ObfMapSectionLevelTreeNode>& treeNode,
            QHash<ObfMapSectionDataBlockId, std::shared_ptr<const ObfMapSectionLevelTreeNode>>& nodeCache,
            QReadWriteLock& nodeCacheAccessMutex,
            int treeDepth,
            MapSurfaceType& outChildrenSurfaceType,
            QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >* nodesWithData,
            const AreaI* bbox31,
            const std::shared_ptr<const IQueryController>& queryController,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric);

        typedef std::function < bool(
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const DataBlockId& blockId,
            const ObfObjectId mapObjectId,
            const AreaI& bbox,
            const ZoomLevel firstZoomLevel,
            const ZoomLevel lastZoomLevel) > FilterReadingByIdFunction;
        static void readMapObjectsBlock(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const MapPresentationEnvironment>& environment,
            MapStyleEvaluator& evaluator,
            MapStyleEvaluationResult& evaluationResult,
            QSet<uint>& filteringGrid,
            const PointD& tileCoords,
            const double tileFactor,
            const std::shared_ptr<const ObfMapSectionLevelTreeNode>& treeNode,
            const DataBlockId& blockId,
            QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
            const AreaI* bbox31,
            const FilterReadingByIdFunction filterById,
            const VisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric,
            bool coastlineOnly);

        static void readMapObjectId(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            uint64_t baseId,
            uint64_t& objectId);

        static void readMapObject(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const MapPresentationEnvironment>& environment,
            MapStyleEvaluator& evaluator,
            MapStyleEvaluationResult& evaluationResult,
            QSet<uint>& filteringGrid,
            const PointD& tileCoords,
            const double tileFactor,
            uint64_t baseId,
            const std::shared_ptr<const ObfMapSectionLevelTreeNode>& treeNode,
            std::shared_ptr<OsmAnd::BinaryMapObject>& mapObjectOut,
            const AreaI* bbox31,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric);

        enum : uint32_t {
            ShiftCoordinates = 5,
            MaskToRead = ~((1u << ShiftCoordinates) - 1),
        };

    public:
        static void loadMapObjects(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const MapPresentationEnvironment>& environment,
            ZoomLevel zoom,
            const AreaI* bbox31,
            QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
            MapSurfaceType* outBBoxOrSectionSurfaceType,
            const FilterByIdFunction filterById,
            const VisitorFunction visitor,
            DataBlocksCache* cache,
            QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries,
            const std::shared_ptr<const IQueryController>& queryController,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric,
            bool coastlineOnly);

    friend class OsmAnd::ObfMapSectionReader;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_P_H_)
