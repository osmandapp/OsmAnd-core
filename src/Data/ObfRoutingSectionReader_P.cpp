#include "ObfRoutingSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"
#include "ObfRoutingSectionReader_Metrics.h"
#include "Road.h"
#include "ObfReaderUtilities.h"
#include "Stopwatch.h"
#include "IQueryController.h"
#include "Utilities.h"

OsmAnd::ObfRoutingSectionReader_P::ObfRoutingSectionReader_P()
{
}

OsmAnd::ObfRoutingSectionReader_P::~ObfRoutingSectionReader_P()
{
}

void OsmAnd::ObfRoutingSectionReader_P::read(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfRoutingSectionInfo>& section)
{
    const auto cis = reader._codedInputStream.get();

    for (;;)
    {
        auto tag = cis->ReadTag();
        auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                return;
            case OBF::OsmAndRoutingIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, section->_name);
                break;
            case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
            case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                cis->Skip(length);
                break;
            }
            case OBF::OsmAndRoutingIndex::kBlocksFieldNumber:
                cis->Skip(cis->BytesUntilLimit());
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readRules(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfRoutingSectionEncodingDecodingRules>& rules)
{
    const auto cis = reader._codedInputStream.get();

    uint32_t defaultRuleId = 1;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        const auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                return;
            case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readLength(cis);
                const auto oldLimit = cis->PushLimit(length);
                readRule(reader, rules, defaultRuleId++);
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readRule(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfRoutingSectionEncodingDecodingRules>& rules,
    const uint32_t defaultId)
{
    const auto cis = reader._codedInputStream.get();

    QString ruleTag;
    QString ruleValue;
    uint32_t ruleId = defaultId;

    for (;;)
    {
        auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                // Normalize some values
                if (ruleValue.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
                    ruleValue = QLatin1String("yes");
                if (ruleValue.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0)
                    ruleValue = QLatin1String("no");

                rules->createRule(ruleId, ruleTag, ruleValue);
                return;
            }
            case OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
                ObfReaderUtilities::readQString(cis, ruleTag);
                break;
            case OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
                ObfReaderUtilities::readQString(cis, ruleValue);
                break;
            case OBF::OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber:
            {
                gpb::uint32 id;
                cis->ReadVarint32(&id);
                ruleId = id;
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readLevelTreeNodes(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfRoutingSectionLevel>& level)
{
    const auto cis = reader._codedInputStream.get();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        const auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                return;
            case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
            case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                if ((level->dataLevel == RoutingDataLevel::Basemap && tfn == OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber) ||
                    (level->dataLevel == RoutingDataLevel::Detailed && tfn == OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber))
                {
                    // Skip other type of data level
                    const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                    cis->Skip(length);
                    break;
                }

                const std::shared_ptr<ObfRoutingSectionLevelTreeNode> treeNode(new ObfRoutingSectionLevelTreeNode());
                treeNode->length = ObfReaderUtilities::readBigEndianInt(cis);
                treeNode->offset = cis->CurrentPosition();

                const auto oldLimit = cis->PushLimit(treeNode->length);
                readLevelTreeNode(reader, treeNode, nullptr);
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

                level->_p->_rootNodes.push_back(qMove(treeNode));
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readLevelTreeNode(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfRoutingSectionLevelTreeNode>& node,
    const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& parentNode)
{
    const auto cis = reader._codedInputStream.get();
    const auto origin = cis->CurrentPosition();

    for (;;)
    {
        const auto lastPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.left() = d + (parentNode ? parentNode->area31.left() : 0);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.right() = d + (parentNode ? parentNode->area31.right() : 0);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.top() = d + (parentNode ? parentNode->area31.top() : 0);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.bottom() = d + (parentNode ? parentNode->area31.bottom() : 0);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            {
                node->dataOffset = origin + ObfReaderUtilities::readBigEndianInt(cis);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                // Save children relative offset and skip their data
                node->childrenRelativeOffset = lastPos - origin;
                cis->Skip(cis->BytesUntilLimit());

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readLevelTreeNodeChildren(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& treeNode,
    QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> >* outNodesWithData,
    const AreaI* bbox31,
    const IQueryController* const controller,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    const auto cis = reader._codedInputStream.get();

    for (;;)
    {
        const auto lastPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                const std::shared_ptr<ObfRoutingSectionLevelTreeNode> childNode(new ObfRoutingSectionLevelTreeNode());

                childNode->length = ObfReaderUtilities::readBigEndianInt(cis);
                childNode->offset = cis->CurrentPosition();

                const auto oldLimit = cis->PushLimit(childNode->length);
                readLevelTreeNode(reader, childNode, treeNode);
                assert(cis->BytesUntilLimit() == 0);
                assert(treeNode->area31.contains(childNode->area31));
                cis->PopLimit(oldLimit);

                // Update metric
                if (metric)
                    metric->visitedNodes++;

                if (bbox31)
                {
                    const auto shouldSkip =
                        !bbox31->contains(childNode->area31) &&
                        !childNode->area31.contains(*bbox31) &&
                        !bbox31->intersects(childNode->area31);
                    if (shouldSkip)
                        break;
                }

                // Update metric
                if (metric)
                    metric->acceptedNodes++;

                if (outNodesWithData && childNode->dataOffset > 0)
                    outNodesWithData->push_back(childNode);

                if (childNode->childrenRelativeOffset > 0)
                {
                    cis->Seek(childNode->offset);
                    const auto oldLimit = cis->PushLimit(childNode->length);

                    cis->Skip(childNode->childrenRelativeOffset);
                    readLevelTreeNodeChildren(reader, childNode, outNodesWithData, bbox31, controller, metric);
                    assert(cis->BytesUntilLimit() == 0);

                    cis->PopLimit(oldLimit);
                }
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readRoadsBlock(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& treeNode,
    QList< std::shared_ptr<const OsmAnd::Model::Road> >* resultOut,
    const AreaI* bbox31,
    const FilterRoadsByIdFunction filterById,
    const VisitorFunction visitor,
    const IQueryController* const controller,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    QStringList roadsCaptionsTable;
    QList<uint64_t> roadsIdsTable;
    QHash< uint32_t, std::shared_ptr<Model::Road> > resultsByInternalId;

    auto cis = reader._codedInputStream.get();
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                for (const auto& road : constOf(resultsByInternalId))
                {
                    // Fill captions of roads from stringtable
                    for (auto& caption : road->_captions)
                    {
                        const uint32_t stringId = ObfReaderUtilities::decodeIntegerFromString(caption);

                        if (stringId >= roadsCaptionsTable.size())
                        {
                            LogPrintf(LogSeverityLevel::Error,
                                "Data mismatch: string #%d (road %s not found in string table (size %d) in section '%s'",
                                stringId,
                                qPrintable(road->id.toString()),
                                roadsCaptionsTable.size(), qPrintable(section->name));
                            caption = QString::fromLatin1("#%1 NOT FOUND").arg(stringId);
                            continue;
                        }
                        caption = roadsCaptionsTable[stringId];
                    }

                    if (!visitor || visitor(road))
                    {
                        if (resultOut)
                            resultOut->push_back(road);
                    }
                }
            }
                return;
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kIdTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);
                readRoadsBlockIdsTable(reader, roadsIdsTable);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kDataObjectsFieldNumber:
            {
                const Stopwatch readRoadStopwatch(metric != nullptr);
                std::shared_ptr<Model::Road> road;
                uint32_t internalId;

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readRoad(reader, section, treeNode, bbox31, filterById, roadsIdsTable, internalId, road, metric);
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

                // Update metric
                if (metric)
                    metric->visitedRoads++;

                // If map object was not read, skip it
                if (!road)
                {
                    if (metric)
                        metric->elapsedTimeForOnlyVisitedRoads += readRoadStopwatch.elapsed();

                    break;
                }

                // Update metric
                if (metric)
                {
                    metric->elapsedTimeForOnlyAcceptedRoads += readRoadStopwatch.elapsed();

                    metric->acceptedRoads++;
                }

                resultsByInternalId.insert(internalId, road);

                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kRestrictionsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readRoadsBlockRestrictions(reader, resultsByInternalId, roadsIdsTable);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                ObfReaderUtilities::readStringTable(cis, roadsCaptionsTable);
                cis->PopLimit(oldLimit);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readRoadsBlockIdsTable(
    const ObfReader_P& reader,
    QList<uint64_t>& ids)
{
    auto cis = reader._codedInputStream.get();

    uint64_t id = 0;

    for (;;)
    {
        auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
            case OBF::IdTable::kRouteIdFieldNumber:
            {
                id += ObfReaderUtilities::readSInt64(cis);
                ids.push_back(id);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readRoadsBlockRestrictions(
    const ObfReader_P& reader,
    const QHash< uint32_t, std::shared_ptr<Model::Road> >& roadsByInternalIds,
    const QList<uint64_t>& roadsInternalIdToGlobalIdMap)
{
    uint32_t originInternalId;
    uint32_t destinationInternalId;
    uint32_t restrictionType;

    auto cis = reader._codedInputStream.get();
    for (;;)
    {
        auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                const auto originRoad = roadsByInternalIds[originInternalId];
                if (!originRoad)
                    return;
                const auto destinationRoadId = roadsInternalIdToGlobalIdMap[destinationInternalId];
                originRoad->_restrictions.insert(destinationRoadId, static_cast<Model::RoadRestriction>(restrictionType));
                return;
            }
            case OBF::RestrictionData::kFromFieldNumber:
            {
                cis->ReadVarint32(&originInternalId);
                break;
            }
            case OBF::RestrictionData::kToFieldNumber:
            {
                cis->ReadVarint32(&destinationInternalId);
                break;
            }
            case OBF::RestrictionData::kTypeFieldNumber:
            {
                cis->ReadVarint32(&restrictionType);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readRoad(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& treeNode,
    const AreaI* bbox31,
    const FilterRoadsByIdFunction filterById,
    const QList<uint64_t>& idsTable,
    uint32_t& internalId,
    std::shared_ptr<Model::Road>& road,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    const auto cis = reader._codedInputStream.get();
    const auto baseOffset = cis->CurrentPosition();

    for (;;)
    {
        auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
            case OBF::RouteData::kPointsFieldNumber:
            {
                const Stopwatch roadPointsStopwatch(metric != nullptr);

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                AreaI roadBBox;
                roadBBox.top() = roadBBox.left() = std::numeric_limits<int32_t>::max();
                roadBBox.bottom() = roadBBox.right() = 0;
                auto lastUnprocessedPointForBBox = 0;

                // In protobuf, a sint32 can be encoded using [1..4] bytes,
                // so try to guess size of array, and preallocate it.
                // (BytesUntilLimit/2) is ~= number of vertices, and is always larger than needed.
                // So it's impossible that a buffer overflow will ever happen. But assert on that.
                const auto probableVerticesCount = (cis->BytesUntilLimit() / 2);
                QVector< PointI > points31(probableVerticesCount);

                bool shouldNotSkip = (bbox31 == nullptr);
                auto pointsCount = 0;
                auto dx = treeNode->area31.left() >> ShiftCoordinates;
                auto dy = treeNode->area31.top() >> ShiftCoordinates;
                auto pPoint = points31.data();
                while (cis->BytesUntilLimit() > 0)
                {
                    const uint32_t x = ObfReaderUtilities::readSInt32(cis) + dx;
                    const uint32_t y = ObfReaderUtilities::readSInt32(cis) + dy;

                    PointI point31(
                        x << ShiftCoordinates,
                        y << ShiftCoordinates);
                    // Save point into storage
                    assert(points31.size() > pointsCount);
                    *(pPoint++) = point31;
                    pointsCount++;

                    dx = x;
                    dy = y;

                    // Check if road should be maintained
                    if (!shouldNotSkip && bbox31)
                    {
                        const Stopwatch roadBboxStopwatch(metric != nullptr);

                        shouldNotSkip = bbox31->contains(point31);
                        roadBBox.enlargeToInclude(point31);

                        if (metric)
                            metric->elapsedTimeForRoadsBbox += roadBboxStopwatch.elapsed();

                        lastUnprocessedPointForBBox = pointsCount;
                    }
                }
                cis->PopLimit(oldLimit);

                // Since reserved space may be larger than actual amount of data,
                // shrink the vertices array
                points31.resize(pointsCount);

                // Even if no point lays inside bbox, an edge
                // may intersect the bbox
                if (!shouldNotSkip && bbox31)
                {
                    assert(lastUnprocessedPointForBBox == points31.size());

                    shouldNotSkip =
                        roadBBox.contains(*bbox31) ||
                        bbox31->intersects(roadBBox);
                }

                // If map object didn't fit, skip it's entire content
                if (!shouldNotSkip)
                {
                    // Update metric
                    if (metric)
                        metric->elapsedTimeForSkippedRoadsPoints += roadPointsStopwatch.elapsed();

                    cis->Skip(cis->BytesUntilLimit());
                    break;
                }

                // Update metric
                if (metric)
                    metric->elapsedTimeForNotSkippedRoadsPoints += roadPointsStopwatch.elapsed();

                // In case bbox is not fully calculated, complete this task
                auto pPointForBBox = points31.data() + lastUnprocessedPointForBBox;
                while (lastUnprocessedPointForBBox < points31.size())
                {
                    const Stopwatch roadBboxStopwatch(metric != nullptr);

                    roadBBox.enlargeToInclude(*pPointForBBox);

                    if (metric)
                        metric->elapsedTimeForRoadsBbox += roadBboxStopwatch.elapsed();

                    lastUnprocessedPointForBBox++;
                    pPointForBBox++;
                }

                // Finally, create the object
                if (!road)
                    road.reset(new OsmAnd::Model::Road(section));
                road->_points31 = qMove(points31);
                road->_bbox31 = roadBBox;

                break;
            }
            case OBF::RouteData::kPointTypesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while (cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 pointIdx;
                    cis->ReadVarint32(&pointIdx);

                    gpb::uint32 innerLength;
                    cis->ReadVarint32(&innerLength);
                    auto innerOldLimit = cis->PushLimit(innerLength);

                    auto& pointTypes = road->_pointsTypes.insert(pointIdx, QVector<uint32_t>()).value();
                    while (cis->BytesUntilLimit() > 0)
                    {
                        gpb::uint32 pointType;
                        cis->ReadVarint32(&pointType);
                        pointTypes.push_back(pointType);
                    }
                    cis->PopLimit(innerOldLimit);
                }
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::RouteData::kTypesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while (cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 type;
                    cis->ReadVarint32(&type);
                    road->_types.push_back(type);
                }
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::RouteData::kRouteIdFieldNumber:
            {
                gpb::uint32 id;
                cis->ReadVarint32(&id);
                internalId = id;
                if (id < idsTable.size())
                    road->_id = ObfObjectId::generateUniqueId(idsTable[id], baseOffset, section);
                else
                    road->_id = ObfObjectId::generateUniqueId(id, baseOffset, section);
                break;
            }
            case OBF::RouteData::kStringNamesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while (cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 stringTag;
                    cis->ReadVarint32(&stringTag);
                    gpb::uint32 stringId;
                    cis->ReadVarint32(&stringId);

                    road->_captions.insert(stringTag, ObfReaderUtilities::encodeIntegerToString(stringId));
                    road->_captionsOrder.push_back(stringTag);
                }
                cis->PopLimit(oldLimit);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::loadRoads(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const RoutingDataLevel dataLevel,
    const AreaI* const bbox31,
    QList< std::shared_ptr<const OsmAnd::Model::Road> >* resultOut,
    const FilterRoadsByIdFunction filterById,
    const VisitorFunction visitor,
    DataBlocksCache* cache,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries,
    const IQueryController* const controller,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    const auto cis = reader._codedInputStream.get();

    // Check if this section has initialized rules
    {
        QMutexLocker scopedLocker(&section->_p->_decodingRulesMutex);

        if (!section->_p->_decodingRules)
        {
            cis->Seek(section->_offset);
            auto oldLimit = cis->PushLimit(section->_length);

            std::shared_ptr<ObfRoutingSectionEncodingDecodingRules> decodingRules(new ObfRoutingSectionEncodingDecodingRules());
            readRules(reader, decodingRules);
            section->_p->_decodingRules = decodingRules;
            assert(cis->BytesUntilLimit() == 0);

            cis->PopLimit(oldLimit);
        }
    }

    ObfRoutingSectionReader_Metrics::Metric_loadRoads localMetric;

    const Stopwatch treeNodesStopwatch(metric != nullptr);

    // Check if tree nodes were loaded for specified data level
    auto& container = section->_p->_levelContainers[static_cast<unsigned int>(dataLevel)];
    {
        QMutexLocker scopedLocker(&container.mutex);

        if (!container.level)
        {
            cis->Seek(section->_offset);
            auto oldLimit = cis->PushLimit(section->_length);

            std::shared_ptr<ObfRoutingSectionLevel> level(new ObfRoutingSectionLevel(dataLevel));
            readLevelTreeNodes(reader, level);
            container.level = level;
            assert(cis->BytesUntilLimit() == 0);

            cis->PopLimit(oldLimit);
        }
    }

    // Collect all tree nodes that contain data
    QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> > treeNodesWithData;
    for (const auto& rootNode : constOf(container.level->rootNodes))
    {
        if (metric)
            metric->visitedNodes++;

        if (bbox31)
        {
            const Stopwatch bboxNodeCheckStopwatch(metric != nullptr);

            const auto shouldSkip =
                !bbox31->contains(rootNode->area31) &&
                !rootNode->area31.contains(*bbox31) &&
                !bbox31->intersects(rootNode->area31);

            // Update metric
            if (metric)
                metric->elapsedTimeForNodesBbox += bboxNodeCheckStopwatch.elapsed();

            if (shouldSkip)
                continue;
        }

        // Update metric
        if (metric)
            metric->acceptedNodes++;

        if (rootNode->dataOffset > 0)
            treeNodesWithData.push_back(rootNode);

        if (rootNode->childrenRelativeOffset > 0)
        {
            cis->Seek(rootNode->offset);
            auto oldLimit = cis->PushLimit(rootNode->length);

            cis->Skip(rootNode->childrenRelativeOffset);
            readLevelTreeNodeChildren(reader, rootNode, &treeNodesWithData, bbox31, controller, metric);
            assert(cis->BytesUntilLimit() == 0);

            cis->PopLimit(oldLimit);
        }
    }


    // Sort blocks by data offset to force forward-only seeking
    qSort(treeNodesWithData.begin(), treeNodesWithData.end(),
        []
        (const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& l, const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& r) -> bool
        {
            return l->dataOffset < r->dataOffset;
        });

    // Update metric
    const Stopwatch roadsStopwatch(metric != nullptr);
    if (metric)
        metric->elapsedTimeForNodes += treeNodesStopwatch.elapsed();

    // Read map objects from their blocks
    QList< std::shared_ptr<const DataBlock> > danglingReferencedCacheEntries;
    for (const auto& treeNode : constOf(treeNodesWithData))
    {
        if (controller && controller->isAborted())
            break;

        DataBlockId blockId;
        blockId.sectionRuntimeGeneratedId = section->runtimeGeneratedId;
        blockId.offset = treeNode->dataOffset;

        if (cache && cache->shouldCacheBlock(blockId, dataLevel, treeNode->area31, bbox31))
        {
            // In case cache is provided, read and cache

            std::shared_ptr<const DataBlock> dataBlock;
            std::shared_ptr<const DataBlock> sharedBlockReference;
            proper::shared_future< std::shared_ptr<const DataBlock> > futureSharedBlockReference;
            if (cache->obtainReferenceOrFutureReferenceOrMakePromise(blockId, sharedBlockReference, futureSharedBlockReference))
            {
                // Got reference or future reference

                // Update metric
                if (metric)
                    metric->roadsBlocksReferenced++;

                if (sharedBlockReference)
                {
                    // Ok, this block was already loaded, just use it
                    dataBlock = sharedBlockReference;
                }
                else
                {
                    // Wait until it will be loaded
                    dataBlock = futureSharedBlockReference.get();
                }
                assert(dataBlock->dataLevel == dataLevel);
            }
            else
            {
                // Made a promise, so load entire block into temporary storage
                QList< std::shared_ptr<const Model::Road> > roads;

                cis->Seek(treeNode->dataOffset);

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                readRoadsBlock(
                    reader,
                    section,
                    treeNode,
                    &roads,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    metric ? &localMetric : nullptr);
                assert(cis->BytesUntilLimit() == 0);

                cis->PopLimit(oldLimit);

                // Update metric
                if (metric)
                    metric->roadsBlocksRead++;

                // Create a data block and share it
                dataBlock.reset(new DataBlock(blockId, dataLevel, treeNode->area31, roads));
                cache->fulfilPromiseAndReference(blockId, dataBlock);
            }

            if (outReferencedCacheEntries)
                outReferencedCacheEntries->push_back(dataBlock);
            else
                danglingReferencedCacheEntries.push_back(dataBlock);

            // Process data block
            for (const auto& road : constOf(dataBlock->roads))
            {
                if (metric)
                    metric->visitedRoads++;

                if (bbox31)
                {
                    const auto shouldNotSkip =
                        road->bbox31.contains(*bbox31) ||
                        bbox31->intersects(road->bbox31);

                    if (!shouldNotSkip)
                        continue;
                }

                // Check if map object is desired
                if (filterById && !filterById(section, road->id, road->bbox31))
                    continue;

                if (!visitor || visitor(road))
                {
                    if (metric)
                        metric->acceptedRoads++;

                    if (resultOut)
                        resultOut->push_back(qMove(road));
                }
            }
        }
        else
        {
            // In case there's no cache, simply read

            cis->Seek(treeNode->dataOffset);

            gpb::uint32 length;
            cis->ReadVarint32(&length);
            const auto oldLimit = cis->PushLimit(length);

            readRoadsBlock(
                reader,
                section,
                treeNode,
                resultOut,
                bbox31,
                filterById,
                visitor,
                controller,
                metric);
            assert(cis->BytesUntilLimit() == 0);

            cis->PopLimit(oldLimit);

            // Update metric
            if (metric)
                metric->roadsBlocksRead++;
        }

        // Update metric
        if (metric)
            metric->roadsBlocksProcessed++;
    }

    // In case cache was supplied, but referenced cache entries output collection was not specified, release all dangling references
    if (cache && !outReferencedCacheEntries)
    {
        for (auto& referencedCacheEntry : danglingReferencedCacheEntries)
            cache->releaseReference(referencedCacheEntry->id, referencedCacheEntry);
        danglingReferencedCacheEntries.clear();
    }

    // Update metric
    if (metric)
        metric->elapsedTimeForRoadsBlocks += roadsStopwatch.elapsed();

    // In case cache was used, and metric was requested, some values must be taken from different parts
    if (cache && metric)
    {
        metric->elapsedTimeForOnlyAcceptedRoads += localMetric.elapsedTimeForOnlyAcceptedRoads;
    }
}
