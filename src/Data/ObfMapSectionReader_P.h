#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QMap>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapTypes.h"
#include "ObfMapSectionReader.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfMapSectionInfo;
    class ObfMapSectionLevel;
    struct ObfMapSectionDecodingEncodingRules;
    class ObfMapSectionLevelTreeNode;
    namespace Model
    {
        class BinaryMapObject;
    }
    class IQueryController;
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects;
    }

    class ObfMapSectionReader;
    class ObfMapSectionReader_P Q_DECL_FINAL
    {
    public:
        typedef ObfMapSectionReader::VisitorFunction VisitorFunction;

    private:
        ObfMapSectionReader_P();
        ~ObfMapSectionReader_P();

    protected:
        static void read(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfMapSectionInfo>& section);

        static void readMapLevelHeader(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<ObfMapSectionLevel>& level);

        static void readEncodingDecodingRules(
            const ObfReader_P& reader,
            const std::shared_ptr<ObfMapSectionDecodingEncodingRules>& encodingDecodingRules);

        static void readEncodingDecodingRule(
            const ObfReader_P& reader,
            uint32_t defaultId,
            const std::shared_ptr<ObfMapSectionDecodingEncodingRules>& encodingDecodingRules);

        static void readMapLevelTreeNodes(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const ObfMapSectionLevel>& level,
            QList< std::shared_ptr<ObfMapSectionLevelTreeNode> >& nodes);

        static void readTreeNode(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const AreaI& parentArea,
            const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode);

        static void readTreeNodeChildren(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode,
            MapFoundationType& foundation,
            QList< std::shared_ptr<ObfMapSectionLevelTreeNode> >* nodesWithData,
            const AreaI* bbox31,
            const IQueryController* const controller,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric);

        static void readMapObjectsBlock(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode,
            QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >* resultOut,
            const AreaI* bbox31,
            const FilterMapObjectsByIdFunction filterById,
            const VisitorFunction visitor,
            const IQueryController* const controller,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric);

        static void readMapObjectId(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            uint64_t baseId,
            uint64_t& objectId);

        static void readMapObject(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            uint64_t baseId,
            const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode,
            std::shared_ptr<OsmAnd::Model::BinaryMapObject>& mapObjectOut,
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
            ZoomLevel zoom,
            const AreaI* bbox31,
            QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> >* resultOut,
            MapFoundationType* foundationOut,
            const FilterMapObjectsByIdFunction filterById,
            const VisitorFunction visitor,
            const IQueryController* const controller,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric);

    friend class OsmAnd::ObfMapSectionReader;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_P_H_)
