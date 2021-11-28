#include "ObfRoutingSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "Nullable.h"
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
    const auto cis = reader.getCodedInputStream().get();

    Nullable<AreaI> bbox31;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;
                if(bbox31.isSet())  {
                    section->area31 = *bbox31;
                } else {
                    AreaI empty;
                    section->area31 = empty;
                }
                return;
            case OBF::OsmAndRoutingIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, section->name);
                break;
            case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
            case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto oldLimit = cis->PushLimit(length);

                AreaI nodeBbox31;
                readLevelTreeNodeBbox31(reader, nodeBbox31);
                if (bbox31.isSet())
                    bbox31->enlargeToInclude(nodeBbox31);
                else
                    bbox31 = nodeBbox31;

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

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

void OsmAnd::ObfRoutingSectionReader_P::readLevelTreeNodeBbox31(
    const ObfReader_P& reader,
    AreaI& outBbox31)
{
    const auto cis = reader.getCodedInputStream().get();

    outBbox31 = AreaI::largestPositive();

    for (;;)
    {
        //const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        const auto tgn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tgn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber:
                outBbox31.topLeft.x = ObfReaderUtilities::readSInt32(cis);
                break;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
                outBbox31.bottomRight.x = ObfReaderUtilities::readSInt32(cis);
                break;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
                outBbox31.topLeft.y = ObfReaderUtilities::readSInt32(cis);
                break;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
                outBbox31.bottomRight.y = ObfReaderUtilities::readSInt32(cis);
                break;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
                cis->Skip(cis->BytesUntilLimit());
                return;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readAttributeMapping(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const std::shared_ptr<ObfRoutingSectionAttributeMapping>& attributeMapping)
{
    const auto cis = reader.getCodedInputStream().get();

    bool atLeastOneRuleRead = false;
    uint32_t naturalId = 1;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        const auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                attributeMapping->verifyRequiredMappingRegistered();
                return;
            case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readLength(cis);
                //const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                readAttributeMappingEntry(reader, naturalId++, attributeMapping);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                atLeastOneRuleRead = true;
                break;
            }
            default:
                if (atLeastOneRuleRead)
                {
                    attributeMapping->verifyRequiredMappingRegistered();
                    return;
                }
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readAttributeMappingEntry(
    const ObfReader_P& reader,
    const uint32_t naturalId,
    const std::shared_ptr<ObfRoutingSectionAttributeMapping>& attributeMapping)
{
    const auto cis = reader.getCodedInputStream().get();

    uint32_t entryId = naturalId;
    QString entryTag;
    QString entryValue;
    
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                attributeMapping->registerMapping(entryId, entryTag, entryValue);
                return;
            case OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
                ObfReaderUtilities::readQString(cis, entryTag);
                break;
            case OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
                ObfReaderUtilities::readQString(cis, entryValue);

                // Normalize some values
                if (entryValue.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
                    entryValue = QLatin1String("yes");
                if (entryValue.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0)
                    entryValue = QLatin1String("no");

                break;
            case OBF::OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&entryId));
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readLevelTreeNodes(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const std::shared_ptr<ObfRoutingSectionLevel>& level)
{
    const auto cis = reader.getCodedInputStream().get();

    bool atLeastOneBasemapRootBoxRead = false;
    bool atLeastOneDetailedRootBoxRead = false;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        const auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
            case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                if (tfn == OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber)
                    atLeastOneBasemapRootBoxRead = true;
                else if (tfn == OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber)
                    atLeastOneDetailedRootBoxRead = true;

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

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                level->_p->_rootNodes.push_back(qMove(treeNode));
                break;
            }
            default:
                if (atLeastOneBasemapRootBoxRead || atLeastOneDetailedRootBoxRead)
                    return;
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
    const auto cis = reader.getCodedInputStream().get();
    const auto origin = cis->CurrentPosition();

    uint64_t fieldsMask = 0u;
    const auto safeToSkipFieldsMask =
        (1ull << OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber) |
        (1ull << OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber) |
        (1ull << OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber) |
        (1ull << OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber);
    bool kBoxesFieldNumberProcessed = false;

    for (;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        const auto tgn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tgn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.left() = d + (parentNode ? parentNode->area31.left() : 0);

                fieldsMask |= (1ull << tgn);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.right() = d + (parentNode ? parentNode->area31.right() : 0);

                fieldsMask |= (1ull << tgn);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.top() = d + (parentNode ? parentNode->area31.top() : 0);

                fieldsMask |= (1ull << tgn);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                node->area31.bottom() = d + (parentNode ? parentNode->area31.bottom() : 0);

                fieldsMask |= (1ull << tgn);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            {
                node->dataOffset = origin + ObfReaderUtilities::readBigEndianInt(cis);

                fieldsMask |= (1ull << tgn);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                if (!kBoxesFieldNumberProcessed)
                {
                    node->hasChildrenDataBoxes = true;
                    node->firstDataBoxInnerOffset = tagPos - node->offset;
                    kBoxesFieldNumberProcessed = true;

                    if (fieldsMask == safeToSkipFieldsMask)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                ObfReaderUtilities::skipUnknownField(cis, tag);
                fieldsMask |= (1ull << tgn);
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
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& treeNode,
    QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> >* outNodesWithData,
    const AreaI* bbox31,
    const std::shared_ptr<const IQueryController>& queryController,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        //const auto lastPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                const std::shared_ptr<ObfRoutingSectionLevelTreeNode> childNode(new ObfRoutingSectionLevelTreeNode());
                childNode->length = ObfReaderUtilities::readBigEndianInt(cis);
                childNode->offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(childNode->length);

                readLevelTreeNode(reader, childNode, treeNode);
                assert(treeNode->area31.contains(childNode->area31));

                ObfReaderUtilities::ensureAllDataWasRead(cis);
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

                if (childNode->hasChildrenDataBoxes)
                {
                    cis->Seek(childNode->offset);
                    const auto oldLimit = cis->PushLimit(childNode->length);

                    cis->Skip(childNode->firstDataBoxInnerOffset);
                    readLevelTreeNodeChildren(reader, section, childNode, outNodesWithData, bbox31, queryController, metric);

                    ObfReaderUtilities::ensureAllDataWasRead(cis);
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
    QList< std::shared_ptr<const OsmAnd::Road> >* resultOut,
    const AreaI* bbox31,
    const FilterRoadsByIdFunction filterById,
    const VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    QStringList roadsCaptionsTable;
    QList<uint64_t> roadsIdsTable;
    QHash< uint32_t, std::shared_ptr<Road> > resultsByInternalId;

    const auto cis = reader.getCodedInputStream().get();
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                for (const auto& road : constOf(resultsByInternalId))
                {
                    // Fill captions of roads from stringtable
                    for (auto& caption : road->captions)
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
                return;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kIdTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                //const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                readRoadsBlockIdsTable(reader, roadsIdsTable);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kDataObjectsFieldNumber:
            {
                const Stopwatch readRoadStopwatch(metric != nullptr);
                std::shared_ptr<Road> road;
                uint32_t internalId;

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                //const auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                readRoad(reader, section, treeNode, bbox31, filterById, roadsIdsTable, internalId, road, metric);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
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
                //const auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                readRoadsBlockRestrictions(reader, resultsByInternalId, roadsIdsTable);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndRoutingIndex_RouteDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                //const auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                ObfReaderUtilities::readStringTable(cis, roadsCaptionsTable);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
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
    const auto cis = reader.getCodedInputStream().get();

    uint64_t id = 0;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

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
    const QHash< uint32_t, std::shared_ptr<Road> >& roadsByInternalIds,
    const QList<uint64_t>& roadsInternalIdToGlobalIdMap)
{
    uint32_t originInternalId;
    uint32_t destinationInternalId;
    uint32_t restrictionType;

    const auto cis = reader.getCodedInputStream().get();
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                const auto originRoad = roadsByInternalIds[originInternalId];
                if (!originRoad)
                    return;
                const auto destinationRoadId = roadsInternalIdToGlobalIdMap[destinationInternalId];
                originRoad->restrictions.insert(ObfObjectId::fromRawId(destinationRoadId), static_cast<RoadRestriction>(restrictionType));
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
    std::shared_ptr<Road>& road,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    const auto cis = reader.getCodedInputStream().get();
    const auto baseOffset = cis->CurrentPosition();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

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
                    road.reset(new OsmAnd::Road(section));
                road->points31 = qMove(points31);
                road->bbox31 = roadBBox;

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

                    auto& pointTypes = road->pointsTypes.insert(pointIdx, QVector<uint32_t>()).value();
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
                    gpb::uint32 attributeId;
                    cis->ReadVarint32(&attributeId);
                    road->attributeIds.push_back(attributeId);
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
                    road->id = ObfObjectId::generateUniqueId(idsTable[id], baseOffset, section);
                else
                    road->id = ObfObjectId::generateUniqueId(id, baseOffset, section);
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

                    road->captions.insert(stringTag, ObfReaderUtilities::encodeIntegerToString(stringId));
                    road->captionsOrder.push_back(stringTag);
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
    QList< std::shared_ptr<const OsmAnd::Road> >* resultOut,
    const FilterRoadsByIdFunction filterById,
    const VisitorFunction visitor,
    DataBlocksCache* cache,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries,
    const std::shared_ptr<const IQueryController>& queryController,
    ObfRoutingSectionReader_Metrics::Metric_loadRoads* const metric)
{
    const auto cis = reader.getCodedInputStream().get();

    // Ensure encoding/decoding rules are read
    if (section->_p->_attributeMappingLoaded.loadAcquire() == 0)
    {
        QMutexLocker scopedLocker(&section->_p->_attributeMappingLoadMutex);
        if (!section->_p->_attributeMapping)
        {
            // Read encoding/decoding rules
            cis->Seek(section->offset);
            auto oldLimit = cis->PushLimit(section->length);

            const std::shared_ptr<ObfRoutingSectionAttributeMapping> attributeMapping(new ObfRoutingSectionAttributeMapping());
            readAttributeMapping(reader, section, attributeMapping);
            section->_p->_attributeMapping = attributeMapping;

            cis->PopLimit(oldLimit);

            section->_p->_attributeMappingLoaded.storeRelease(1);
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
            cis->Seek(section->offset);
            auto oldLimit = cis->PushLimit(section->length);

            std::shared_ptr<ObfRoutingSectionLevel> level(new ObfRoutingSectionLevel(dataLevel));
            readLevelTreeNodes(reader, section, level);
            container.level = level;

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

        if (rootNode->hasChildrenDataBoxes)
        {
            cis->Seek(rootNode->offset);
            auto oldLimit = cis->PushLimit(rootNode->length);

            cis->Skip(rootNode->firstDataBoxInnerOffset);
            readLevelTreeNodeChildren(reader, section, rootNode, &treeNodesWithData, bbox31, queryController, metric);

            ObfReaderUtilities::ensureAllDataWasRead(cis);
            cis->PopLimit(oldLimit);
        }
    }


    // Sort blocks by data offset to force forward-only seeking
    std::sort(treeNodesWithData,
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
        if (queryController && queryController->isAborted())
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
                QList< std::shared_ptr<const Road> > roads;

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

                ObfReaderUtilities::ensureAllDataWasRead(cis);
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
                queryController,
                metric);

            ObfReaderUtilities::ensureAllDataWasRead(cis);
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

void OsmAnd::ObfRoutingSectionReader_P::loadTreeNodes(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const RoutingDataLevel dataLevel,
    QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> >* resultOut)
{
    const auto cis = reader.getCodedInputStream().get();
    
    // Check if tree nodes were loaded for specified data level
    auto& container = section->_p->_levelContainers[static_cast<unsigned int>(dataLevel)];
    {
        QMutexLocker scopedLocker(&container.mutex);
        
        if (!container.level)
        {
            cis->Seek(section->offset);
            auto oldLimit = cis->PushLimit(section->length);
            
            std::shared_ptr<ObfRoutingSectionLevel> level(new ObfRoutingSectionLevel(dataLevel));
            readLevelTreeNodes(reader, section, level);
            container.level = level;
            
            cis->PopLimit(oldLimit);
        }
    }
    
    // Collect all tree nodes that contain data
    for (const auto& rootNode : constOf(container.level->rootNodes))
        resultOut->push_back(rootNode);
}
