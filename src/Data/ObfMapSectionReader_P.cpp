#include "ObfMapSectionReader_P.h"
#include "ObfMapSectionReader.h"
#include "ObfMapSectionReader_Metrics.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "google/protobuf/wire_format_lite.cc"
#include "restore_internal_warnings.h"

#include "Common.h"
#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"
#include "ObfReaderUtilities.h"
#include "BinaryMapObject.h"
#include "IQueryController.h"
#include "Stopwatch.h"
#include "Logging.h"
#include "Utilities.h"

using google::protobuf::internal::WireFormatLite;

OsmAnd::ObfMapSectionReader_P::ObfMapSectionReader_P()
{
}

OsmAnd::ObfMapSectionReader_P::~ObfMapSectionReader_P()
{
}

void OsmAnd::ObfMapSectionReader_P::read(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfMapSectionInfo>& section)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndMapIndex::kNameFieldNumber:
            {
                ObfReaderUtilities::readQString(cis, section->name);
                section->isBasemap = section->name.contains(QLatin1String("basemap"), Qt::CaseInsensitive);
                section->isBasemapWithCoastlines = section->name == QLatin1String("basemap");
                section->isContourLines = section->name.contains(QLatin1String("contour"), Qt::CaseInsensitive);
                break;
            }
            case OBF::OsmAndMapIndex::kRulesFieldNumber:
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::OsmAndMapIndex::kLevelsFieldNumber:
            {
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                Ref<ObfMapSectionLevel> levelRoot(new ObfMapSectionLevel());
                levelRoot->length = length;
                levelRoot->offset = offset;
                readMapLevelHeader(reader, levelRoot);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                section->levels.push_back(qMove(levelRoot));
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readAttributeMapping(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionAttributeMapping>& attributeMapping)
{
    const auto cis = reader.getCodedInputStream().get();

    bool atLeastOneRuleWasRead = false;
    uint32_t naturalId = 1;
    for (;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                attributeMapping->verifyRequiredMappingRegistered();
                return;
            case OBF::OsmAndMapIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                readAttributeMappingEntry(reader, naturalId++, attributeMapping);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                atLeastOneRuleWasRead = true;
                break;
            }
            default:
                if (atLeastOneRuleWasRead)
                {
                    attributeMapping->verifyRequiredMappingRegistered();
                    return;
                }
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readAttributeMappingEntry(
    const ObfReader_P& reader,
    const uint32_t naturalId,
    const std::shared_ptr<ObfMapSectionAttributeMapping>& attributeMapping)
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
            case OBF::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
                ObfReaderUtilities::readQString(cis, entryValue);
                break;
            case OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
                ObfReaderUtilities::readQString(cis, entryTag);
                break;
            case OBF::OsmAndMapIndex_MapEncodingRule::kIdFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&entryId));
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapLevelHeader(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfMapSectionLevel>& level)
{
    const auto cis = reader.getCodedInputStream().get();

    uint64_t fieldsMask = 0u;
    const auto safeToSkipFieldsMask =
        (1ull << OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber);
    bool kBoxesFieldNumberProcessed = false;

    for (;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        const auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->maxZoom));
                fieldsMask |= (1ull << tfn);
                break;
            case OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->minZoom));
                fieldsMask |= (1ull << tfn);
                break;
            case OBF::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->area31.left()));
                fieldsMask |= (1ull << tfn);
                break;
            case OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->area31.right()));
                fieldsMask |= (1ull << tfn);
                break;
            case OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->area31.top()));
                fieldsMask |= (1ull << tfn);
                break;
            case OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->area31.bottom()));
                fieldsMask |= (1ull << tfn);
                break;
            case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
                if (!kBoxesFieldNumberProcessed)
                {
                    level->firstDataBoxInnerOffset = tagPos - level->offset;
                    kBoxesFieldNumberProcessed = true;

                    if (fieldsMask == safeToSkipFieldsMask)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                ObfReaderUtilities::skipUnknownField(cis, tag);
                fieldsMask |= (1ull << tfn);
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapLevelTreeNodes(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<const ObfMapSectionLevel>& level,
    QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >& trees)
{
    const auto cis = reader.getCodedInputStream().get();

    bool atLeastOneMapRootLevelRead = false;
    for (;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfMapSectionLevelTreeNode> levelTree(new ObfMapSectionLevelTreeNode(level));
                levelTree->offset = offset;
                levelTree->length = length;
                readTreeNode(reader, section, level->area31, levelTree);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                trees.push_back(qMove(levelTree));

                atLeastOneMapRootLevelRead = true;
                break;
            }
            default:
                if (atLeastOneMapRootLevelRead)
                    return;
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readTreeNode(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const AreaI& parentArea,
    const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode)
{
    const auto cis = reader.getCodedInputStream().get();

    uint64_t fieldsMask = 0u;
    const auto safeToSkipFieldsMask =
        (1ull << OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber) |
        (1ull << OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber);
    bool kBoxesFieldNumberProcessed = false;

    for (;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        const auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tfn)
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->area31.left() = d + parentArea.left();

                fieldsMask |= (1ull << tfn);
                break;
            }
            case OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->area31.right() = d + parentArea.right();

                fieldsMask |= (1ull << tfn);
                break;
            }
            case OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->area31.top() = d + parentArea.top();

                fieldsMask |= (1ull << tfn);
                break;
            }
            case OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->area31.bottom() = d + parentArea.bottom();

                fieldsMask |= (1ull << tfn);
                break;
            }
            case OBF::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
            {
                const auto offset = ObfReaderUtilities::readBigEndianInt(cis);
                treeNode->dataOffset = offset + treeNode->offset;

                fieldsMask |= (1ull << tfn);
                break;
            }
            case OBF::OsmAndMapIndex_MapDataBox::kOceanFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);

                treeNode->surfaceType = (value != 0) ? MapSurfaceType::FullWater : MapSurfaceType::FullLand;
                assert(
                    (treeNode->surfaceType != MapSurfaceType::FullWater) ||
                    (treeNode->surfaceType == MapSurfaceType::FullWater && section->isBasemapWithCoastlines));

                fieldsMask |= (1ull << tfn);
                break;
            }
            case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            {
                if (!kBoxesFieldNumberProcessed)
                {
                    treeNode->hasChildrenDataBoxes = true;
                    treeNode->firstDataBoxInnerOffset = tagPos - treeNode->offset;
                    kBoxesFieldNumberProcessed = true;

                    if (fieldsMask == safeToSkipFieldsMask)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                ObfReaderUtilities::skipUnknownField(cis, tag);
                fieldsMask |= (1ull << tfn);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readTreeNodeChildren(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<const ObfMapSectionLevelTreeNode>& treeNode,
    MapSurfaceType& outChildrenSurfaceType,
    QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >* nodesWithData,
    const AreaI* bbox31,
    const std::shared_ptr<const IQueryController>& queryController,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader.getCodedInputStream().get();

    outChildrenSurfaceType = MapSurfaceType::Undefined;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfMapSectionLevelTreeNode> childNode(new ObfMapSectionLevelTreeNode(treeNode->level));
                childNode->surfaceType = treeNode->surfaceType;
                childNode->offset = offset;
                childNode->length = length;
                readTreeNode(reader, section, treeNode->area31, childNode);

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

                if (nodesWithData && childNode->dataOffset > 0)
                    nodesWithData->push_back(childNode);

                auto subchildrenSurfaceType = MapSurfaceType::Undefined;
                if (childNode->hasChildrenDataBoxes)
                {
                    cis->Seek(childNode->offset);
                    const auto oldLimit = cis->PushLimit(childNode->length);

                    cis->Skip(childNode->firstDataBoxInnerOffset);
                    readTreeNodeChildren(reader, section, childNode, subchildrenSurfaceType, nodesWithData, bbox31, queryController, metric);

                    ObfReaderUtilities::ensureAllDataWasRead(cis);
                    cis->PopLimit(oldLimit);
                }

                const auto surfaceTypeToMerge = (subchildrenSurfaceType != MapSurfaceType::Undefined) ? subchildrenSurfaceType : childNode->surfaceType;
                if (surfaceTypeToMerge != MapSurfaceType::Undefined)
                {
                    if (outChildrenSurfaceType == MapSurfaceType::Undefined)
                        outChildrenSurfaceType = surfaceTypeToMerge;
                    else if (outChildrenSurfaceType != surfaceTypeToMerge)
                        outChildrenSurfaceType = MapSurfaceType::Mixed;
                }

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapObjectsBlock(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<const ObfMapSectionLevelTreeNode>& tree,
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
    const AreaI* bbox31,
    const FilterReadingByIdFunction filterById,
    const VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader.getCodedInputStream().get();

    QList< std::shared_ptr<BinaryMapObject> > intermediateResult;
    QStringList mapObjectsCaptionsTable;
    gpb::uint64 baseId = 0;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                for (const auto& mapObject : constOf(intermediateResult))
                {
                    // Fill mapObject captions from string-table
                    for (auto& caption : mapObject->captions)
                    {
                        const auto stringId = ObfReaderUtilities::decodeIntegerFromString(caption);

                        if (stringId >= mapObjectsCaptionsTable.size())
                        {
                            LogPrintf(LogSeverityLevel::Error,
                                "Data mismatch: string #%d (map object %s not found in string table (size %d) in section '%s'",
                                stringId,
                                qPrintable(mapObject->id.toString()),
                                mapObjectsCaptionsTable.size(), qPrintable(section->name));
                            caption = QString::fromLatin1("#%1 NOT FOUND").arg(stringId);
                            continue;
                        }
                        caption = mapObjectsCaptionsTable[stringId];
                    }

                    //////////////////////////////////////////////////////////////////////////
                    //if (mapObject->id.getOsmId() == 49048972u)
                    //{
                    //    int i = 5;
                    //}
                    //////////////////////////////////////////////////////////////////////////

                    if (!visitor || visitor(mapObject))
                    {
                        if (resultOut)
                            resultOut->push_back(qMove(mapObject));
                    }
                }

                return;
            }
            case OBF::MapDataBlock::kBaseIdFieldNumber:
            {
                cis->ReadVarint64(&baseId);
                //////////////////////////////////////////////////////////////////////////
                //if (bbox31)
                //    LogPrintf(LogSeverityLevel::Debug, "BBOX %d %d %d %d - MAP BLOCK %" PRIi64, bbox31->top, bbox31->left, bbox31->bottom, bbox31->right, baseId);
                //////////////////////////////////////////////////////////////////////////
                break;
            }
            case OBF::MapDataBlock::kDataObjectsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();

                // Read map object content
                const Stopwatch readMapObjectStopwatch(metric != nullptr);
                std::shared_ptr<OsmAnd::BinaryMapObject> mapObject;
                auto oldLimit = cis->PushLimit(length);
                
                readMapObject(reader, section, baseId, tree, mapObject, bbox31, metric);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                // Update metric
                if (metric)
                    metric->visitedMapObjects++;

                // If map object was not read, skip it
                if (!mapObject)
                {
                    if (metric)
                        metric->elapsedTimeForOnlyVisitedMapObjects += readMapObjectStopwatch.elapsed();

                    break;
                }

                // Update metric
                if (metric)
                {
                    metric->elapsedTimeForOnlyAcceptedMapObjects += readMapObjectStopwatch.elapsed();

                    metric->acceptedMapObjects++;
                }

                //////////////////////////////////////////////////////////////////////////
                //if (mapObject->id.getOsmId() == 49048972u)
                //{
                //    int i = 5;
                //}
                //////////////////////////////////////////////////////////////////////////

                // Check if map object is desired
                const auto shouldReject = filterById && !filterById(
                    section,
                    mapObject->id,
                    mapObject->bbox31,
                    mapObject->level->minZoom,
                    mapObject->level->maxZoom);
                if (shouldReject)
                    break;

                // Save object
                intermediateResult.push_back(qMove(mapObject));

                break;
            }
            case OBF::MapDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                if (intermediateResult.isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    cis->PopLimit(oldLimit);
                    break;
                }
                
                ObfReaderUtilities::readStringTable(cis, mapObjectsCaptionsTable);

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

void OsmAnd::ObfMapSectionReader_P::readMapObject(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    uint64_t baseId,
    const std::shared_ptr<const ObfMapSectionLevelTreeNode>& treeNode,
    std::shared_ptr<OsmAnd::BinaryMapObject>& mapObject,
    const AreaI* bbox31,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader.getCodedInputStream().get();
    const auto baseOffset = cis->CurrentPosition();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        const auto tgn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tgn)
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (mapObject && mapObject->points31.isEmpty())
                {
                    LogPrintf(LogSeverityLevel::Warning,
                        "Empty BinaryMapObject %s detected in section '%s'",
                        qPrintable(mapObject->id.toString()),
                        qPrintable(section->name));
                    mapObject.reset();
                }

                return;
            }
            case OBF::MapData::kAreaCoordinatesFieldNumber:
            case OBF::MapData::kCoordinatesFieldNumber:
            {
                const Stopwatch mapObjectPointsStopwatch(metric != nullptr);

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                PointI p;
                p.x = treeNode->area31.left() & MaskToRead;
                p.y = treeNode->area31.top() & MaskToRead;

                AreaI objectBBox;
                objectBBox.top() = objectBBox.left() = std::numeric_limits<int32_t>::max();
                objectBBox.bottom() = objectBBox.right() = 0;
                auto lastUnprocessedVertexForBBox = 0;

                // In protobuf, a sint32 can be encoded using [1..4] bytes,
                // so try to guess size of array, and preallocate it.
                // (BytesUntilLimit/2) is ~= number of vertices, and is always larger than needed.
                // So it's impossible that a buffer overflow will ever happen. But assert on that.
                const auto probableVerticesCount = (cis->BytesUntilLimit() / 2);
                QVector< PointI > points31(probableVerticesCount);

                auto pPoint = points31.data();
                auto verticesCount = 0;
                bool shouldNotSkip = (bbox31 == nullptr);
                while (cis->BytesUntilLimit() > 0)
                {
                    PointI d;
                    d.x = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
                    d.y = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);

                    p += d;

                    // Save point into storage
                    assert(points31.size() > verticesCount);
                    *(pPoint++) = p;
                    verticesCount++;

                    // Check if map object should be maintained
                    if (!shouldNotSkip && bbox31)
                    {
                        const Stopwatch mapObjectBboxStopwatch(metric != nullptr);

                        shouldNotSkip = bbox31->contains(p);
                        objectBBox.enlargeToInclude(p);

                        if (metric)
                            metric->elapsedTimeForMapObjectsBbox += mapObjectBboxStopwatch.elapsed();

                        lastUnprocessedVertexForBBox = verticesCount;
                    }
                }

                cis->PopLimit(oldLimit);

                // Since reserved space may be larger than actual amount of data,
                // shrink the vertices array
                points31.resize(verticesCount);

                // If map object has no vertices, retain it in a special way to report later, when
                // it's identifier will be known
                if (points31.isEmpty())
                {
                    // Fake that this object is inside bbox
                    shouldNotSkip = true;
                    objectBBox = treeNode->area31;
                }

                // Even if no vertex lays inside bbox, an edge
                // may intersect the bbox
                if (!shouldNotSkip && bbox31)
                {
                    assert(lastUnprocessedVertexForBBox == points31.size());

                    shouldNotSkip =
                        objectBBox.contains(*bbox31) ||
                        bbox31->intersects(objectBBox);
                }

                // If map object didn't fit, skip it's entire content
                if (!shouldNotSkip)
                {
                    if (metric)
                    {
                        metric->elapsedTimeForSkippedMapObjectsPoints += mapObjectPointsStopwatch.elapsed();
                        metric->skippedMapObjectsPoints += points31.size();
                    }

                    cis->Skip(cis->BytesUntilLimit());
                    break;
                }

                // Update metric
                if (metric)
                {
                    metric->elapsedTimeForNotSkippedMapObjectsPoints += mapObjectPointsStopwatch.elapsed();
                    metric->notSkippedMapObjectsPoints += points31.size();
                }

                // In case bbox is not fully calculated, complete this task
                auto pPointForBBox = points31.data() + lastUnprocessedVertexForBBox;
                while (lastUnprocessedVertexForBBox < points31.size())
                {
                    const Stopwatch mapObjectBboxStopwatch(metric != nullptr);

                    objectBBox.enlargeToInclude(*pPointForBBox);

                    if (metric)
                        metric->elapsedTimeForMapObjectsBbox += mapObjectBboxStopwatch.elapsed();

                    lastUnprocessedVertexForBBox++;
                    pPointForBBox++;
                }

                // Finally, create the object
                if (!mapObject)
                    mapObject.reset(new OsmAnd::BinaryMapObject(section, treeNode->level));
                mapObject->isArea = (tgn == OBF::MapData::kAreaCoordinatesFieldNumber);
                mapObject->points31 = qMove(points31);
                mapObject->bbox31 = objectBBox;
                assert(treeNode->area31.top() - mapObject->bbox31.top() <= 32);
                assert(treeNode->area31.left() - mapObject->bbox31.left() <= 32);
                assert(mapObject->bbox31.bottom() - treeNode->area31.bottom() <= 1);
                assert(mapObject->bbox31.right() - treeNode->area31.right() <= 1);
                assert(mapObject->bbox31.right() >= mapObject->bbox31.left());
                assert(mapObject->bbox31.bottom() >= mapObject->bbox31.top());

                break;
            }
            case OBF::MapData::kPolygonInnerCoordinatesFieldNumber:
            {
                if (!mapObject)
                    mapObject.reset(new OsmAnd::BinaryMapObject(section, treeNode->level));

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                PointI p;
                p.x = treeNode->area31.left() & MaskToRead;
                p.y = treeNode->area31.top() & MaskToRead;

                // Preallocate memory
                const auto probableVerticesCount = (cis->BytesUntilLimit() / 2);
                mapObject->innerPolygonsPoints31.push_back(qMove(QVector< PointI >(probableVerticesCount)));
                auto& polygon = mapObject->innerPolygonsPoints31.last();

                auto pPoint = polygon.data();
                auto verticesCount = 0;
                while (cis->BytesUntilLimit() > 0)
                {
                    PointI d;
                    d.x = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
                    d.y = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);

                    p += d;

                    // Save point into storage
                    assert(polygon.size() > verticesCount);
                    *(pPoint++) = p;
                    verticesCount++;
                }

                // Shrink memory
                polygon.resize(verticesCount);

                cis->PopLimit(oldLimit);

                break;
            }
            case OBF::MapData::kAdditionalTypesFieldNumber:
            case OBF::MapData::kTypesFieldNumber:
            {
                if (!mapObject)
                    mapObject.reset(new OsmAnd::BinaryMapObject(section, treeNode->level));

                auto& attributeIds = (tgn == OBF::MapData::kAdditionalTypesFieldNumber)
                    ? mapObject->additionalAttributeIds
                    : mapObject->attributeIds;

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                // Preallocate space
                attributeIds.reserve(cis->BytesUntilLimit());

                while (cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 attributeId;
                    cis->ReadVarint32(&attributeId);

                    attributeIds.push_back(attributeId);
                }

                // Shrink preallocated space
                attributeIds.squeeze();

                cis->PopLimit(oldLimit);

                break;
            }
            case OBF::MapData::kLabelcoordinatesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                u_int i = 0;
                while (cis->BytesUntilLimit() > 0) {
                    if (i == 0) {
                        WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(cis, &mapObject->labelX);
                    } else if (i == 1) {
                        WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(cis, &mapObject->labelY);
                    } else {
                        break;
                    }
                    i++;
                }
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::MapData::kStringNamesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                while (cis->BytesUntilLimit() > 0)
                {
                    bool ok;

                    gpb::uint32 stringRuleId;
                    ok = cis->ReadVarint32(&stringRuleId);
                    assert(ok);
                    gpb::uint32 stringId;
                    ok = cis->ReadVarint32(&stringId);
                    assert(ok);

                    mapObject->captions.insert(stringRuleId, qMove(ObfReaderUtilities::encodeIntegerToString(stringId)));
                    mapObject->captionsOrder.push_back(stringRuleId);
                }

                cis->PopLimit(oldLimit);

                break;
            }
            case OBF::MapData::kIdFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt64(cis);
                const auto rawId = static_cast<uint64_t>(d + baseId);
                mapObject->id = ObfObjectId::generateUniqueId(rawId, baseOffset, section);

                //////////////////////////////////////////////////////////////////////////
                //if (mapObject->id.getOsmId() == 49048972u)
                //{
                //    int i = 5;
                //}
                //////////////////////////////////////////////////////////////////////////

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

bool OsmAnd::ObfMapSectionReader_P::isCoastline(const std::shared_ptr<const BinaryMapObject> & mObj) {
    return mObj && mObj->containsAttribute(mObj->attributeMapping->naturalCoastlineAttributeId);
}

QList< std::shared_ptr<const OsmAnd::BinaryMapObject>> OsmAnd::ObfMapSectionReader_P::filterCoastline(QList< std::shared_ptr<const BinaryMapObject>> & list)
{
    QList< std::shared_ptr<const BinaryMapObject> > mapObjects;
    for (const auto & bmo : list)
    {
        if (isCoastline(bmo))
        {
            mapObjects.push_back(qMove(bmo));
        }
    }    
    return mapObjects;
}

void OsmAnd::ObfMapSectionReader_P::loadMapObjects(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    ZoomLevel zoom,
    const AreaI* bbox31,
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
    MapSurfaceType* outBBoxOrSectionSurfaceType,
    const ObfMapSectionReader::FilterByIdFunction filterById,
    const VisitorFunction visitor,
    DataBlocksCache* cache,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries,
    const std::shared_ptr<const IQueryController>& queryController,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader.getCodedInputStream().get();

    // expand bbox for read coastline in bigger tile (zoom=14)
    // TODO: temporarily disabled since it causes freeze while rendering tiles.
    // Looks like cache->shouldCacheBlock is skipped when it should not.
    const bool isExpandedBBox = false;//zoom >= ZoomLevel::ZoomLevel14;
    const AreaI *expandBbox31 = nullptr;
    AreaI expBbox;
    if (bbox31 && isExpandedBBox)
    {
        expBbox = Utilities::roundBoundingBox31(*bbox31, ZoomLevel::ZoomLevel14);
        expandBbox31 = &expBbox;
    }
    else
    {
        expandBbox31 = bbox31;
    }
    
    const auto filterReadById =
        [filterById, zoom]
        (const std::shared_ptr<const ObfMapSectionInfo>& section,
        const ObfObjectId mapObjectId,
        const AreaI& bbox,
        const ZoomLevel firstZoomLevel,
        const ZoomLevel lastZoomLevel) -> bool
        {
            return filterById(section, mapObjectId, bbox, firstZoomLevel, lastZoomLevel, zoom);
        };

    // Ensure encoding/decoding rules are read
    if (section->_p->_attributeMappingLoaded.loadAcquire() == 0)
    {
        QMutexLocker scopedLocker(&section->_p->_attributeMappingLoadMutex);
        if (!section->_p->_attributeMapping)
        {
            // Read encoding/decoding rules
            cis->Seek(section->offset);
            auto oldLimit = cis->PushLimit(section->length);

            const std::shared_ptr<ObfMapSectionAttributeMapping> attributeMapping(new ObfMapSectionAttributeMapping());
            readAttributeMapping(reader, section, attributeMapping);
            section->_p->_attributeMapping = attributeMapping;

            cis->PopLimit(oldLimit);

            section->_p->_attributeMappingLoaded.storeRelease(1);
        }
    }

    ObfMapSectionReader_Metrics::Metric_loadMapObjects localMetric;

    auto bboxOrSectionSurfaceType = MapSurfaceType::Undefined;
    if (outBBoxOrSectionSurfaceType)
        *outBBoxOrSectionSurfaceType = bboxOrSectionSurfaceType;
    QList< std::shared_ptr<const DataBlock> > danglingReferencedCacheEntries;
    for (const auto& mapLevel : constOf(section->levels))
    {
        // Update metric
        if (metric)
            metric->visitedLevels++;

        if (mapLevel->minZoom > zoom || mapLevel->maxZoom < zoom)
            continue;

        if (expandBbox31)
        {
            bool shouldSkip =
                !expandBbox31->contains(mapLevel->area31) &&
                !mapLevel->area31.contains(*expandBbox31) &&
                !expandBbox31->intersects(mapLevel->area31);

            if (shouldSkip)
                continue;
        }

        bool levelSkip = false;
        if (bbox31)
        {
            const Stopwatch bboxLevelCheckStopwatch(metric != nullptr);

            levelSkip =
                !bbox31->contains(mapLevel->area31) &&
                !mapLevel->area31.contains(*bbox31) &&
                !bbox31->intersects(mapLevel->area31);

            if (metric)
                metric->elapsedTimeForLevelsBbox += bboxLevelCheckStopwatch.elapsed();
        }

        const Stopwatch treeNodesStopwatch(metric != nullptr);
        if (metric)
            metric->acceptedLevels++;

        // If there are no tree nodes in map level, it means they are not loaded.
        // Since loading may be called from multiple threads, loading of root nodes needs synchronization
        // Ensure encoding/decoding rules are read
        if (mapLevel->_p->_rootNodesLoaded.loadAcquire() == 0)
        {
            QMutexLocker scopedLocker(&mapLevel->_p->_rootNodesLoadMutex);
            if (!mapLevel->_p->_rootNodes)
            {
                cis->Seek(mapLevel->offset);
                auto oldLimit = cis->PushLimit(mapLevel->length);

                cis->Skip(mapLevel->firstDataBoxInnerOffset);
                const std::shared_ptr< QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> > > rootNodes(
                    new QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >());
                readMapLevelTreeNodes(reader, section, mapLevel, *rootNodes);
                mapLevel->_p->_rootNodes = rootNodes;

                cis->PopLimit(oldLimit);

                mapLevel->_p->_rootNodesLoaded.storeRelease(1);
            }
        }

        // Collect tree nodes with data
        QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> > treeNodesWithData;
        for (const auto& rootNode : constOf(*mapLevel->_p->_rootNodes))
        {
            // Update metric
            if (metric)
                metric->visitedNodes++;

            if (expandBbox31)
            {
                const Stopwatch bboxNodeCheckStopwatch(metric != nullptr);
                
                const auto shouldSkip =
                    !expandBbox31->contains(rootNode->area31) &&
                    !rootNode->area31.contains(*expandBbox31) &&
                    !expandBbox31->intersects(rootNode->area31);
                
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

            auto rootSubnodesSurfaceType = MapSurfaceType::Undefined;
            if (rootNode->hasChildrenDataBoxes)
            {
                cis->Seek(rootNode->offset);
                auto oldLimit = cis->PushLimit(rootNode->length);

                cis->Skip(rootNode->firstDataBoxInnerOffset);
                readTreeNodeChildren(reader, section, rootNode, rootSubnodesSurfaceType, &treeNodesWithData, expandBbox31, queryController, metric);
                
                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
            }

            const auto surfaceTypeToMerge = (rootSubnodesSurfaceType != MapSurfaceType::Undefined) ? rootSubnodesSurfaceType : rootNode->surfaceType;
            if (surfaceTypeToMerge != MapSurfaceType::Undefined)
            {
                if (bboxOrSectionSurfaceType == MapSurfaceType::Undefined)
                    bboxOrSectionSurfaceType = surfaceTypeToMerge;
                else if (bboxOrSectionSurfaceType != surfaceTypeToMerge)
                    bboxOrSectionSurfaceType = MapSurfaceType::Mixed;
            }
        }

        // Sort blocks by data offset to force forward-only seeking
        std::sort(treeNodesWithData,
            []
            (const std::shared_ptr<const ObfMapSectionLevelTreeNode>& l, const std::shared_ptr<const ObfMapSectionLevelTreeNode>& r) -> bool
            {
                return l->dataOffset < r->dataOffset;
            });

        // Update metric
        const Stopwatch mapObjectsStopwatch(metric != nullptr);
        if (metric)
            metric->elapsedTimeForNodes += treeNodesStopwatch.elapsed();

        // Read map objects from their blocks
        for (const auto& treeNode : constOf(treeNodesWithData))
        {
            if (queryController && queryController->isAborted())
                break;

            DataBlockId blockId;
            blockId.sectionRuntimeGeneratedId = section->runtimeGeneratedId;
            blockId.offset = treeNode->dataOffset;

            bool isCoastlineObjects = isExpandedBBox && levelSkip;
            if (cache && cache->shouldCacheBlock(blockId, treeNode->area31, bbox31) && !isCoastlineObjects)
            {
                // In case cache is provided, read and cache
                const auto levelZooms = Utilities::enumerateZoomLevels(treeNode->level->minZoom, treeNode->level->maxZoom);

                std::shared_ptr<const DataBlock> dataBlock;
                std::shared_ptr<const DataBlock> sharedBlockReference;
                proper::shared_future< std::shared_ptr<const DataBlock> > futureSharedBlockReference;
                if (cache->obtainReferenceOrFutureReferenceOrMakePromise(blockId, zoom, levelZooms, sharedBlockReference, futureSharedBlockReference))
                {
                    // Got reference or future reference

                    // Update metric
                    if (metric)
                        metric->mapObjectsBlocksReferenced++;

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
                }
                else
                {
                    // Made a promise, so load entire block into temporary storage
                    QList< std::shared_ptr<const BinaryMapObject> > mapObjects;

                    cis->Seek(treeNode->dataOffset);

                    gpb::uint32 length;
                    cis->ReadVarint32(&length);
                    const auto oldLimit = cis->PushLimit(length);

                    readMapObjectsBlock(
                        reader,
                        section,
                        treeNode,
                        &mapObjects,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        metric ? &localMetric : nullptr);

                    ObfReaderUtilities::ensureAllDataWasRead(cis);
                    cis->PopLimit(oldLimit);

                    // Update metric
                    if (metric)
                        metric->mapObjectsBlocksRead++;
                   
                    // Create a data block and share it
                    dataBlock.reset(new DataBlock(blockId, treeNode->area31, treeNode->surfaceType, mapObjects));
                    cache->fulfilPromiseAndReference(blockId, levelZooms, dataBlock);                    
                }

                if (outReferencedCacheEntries)
                    outReferencedCacheEntries->push_back(dataBlock);
                else
                    danglingReferencedCacheEntries.push_back(dataBlock);

                // Process data block
                for (const auto& mapObject : constOf(dataBlock->mapObjects))
                {
                    //////////////////////////////////////////////////////////////////////////
                    //if (mapObject->id.getOsmId() == 49048972u)
                    //{
                    //    if (bbox31 && *bbox31 == Utilities::tileBoundingBox31(TileId::fromXY(1052, 673), ZoomLevel11))
                    //    {
                    //        const auto t = mapObject->toString();
                    //        int i = 5;
                    //    }
                    //}
                    //////////////////////////////////////////////////////////////////////////

                    if (metric)
                        metric->visitedMapObjects++;

                    if (expandBbox31)
                    {
                        bool shouldNotSkip =
                            mapObject->bbox31.contains(*expandBbox31) ||
                            expandBbox31->intersects(mapObject->bbox31);

                        if (!shouldNotSkip)
                        {
                            continue;
                        }
                    }

                    if (bbox31)
                    {
                        const auto shouldNotSkip =
                            mapObject->bbox31.contains(*bbox31) ||
                            bbox31->intersects(mapObject->bbox31);

                        if (!shouldNotSkip && !isCoastline(mapObject))
                            continue;
                    }

                    // Check if map object is desired
                    const auto shouldReject = filterById && !filterById(
                        section,
                        mapObject->id,
                        mapObject->bbox31,
                        mapObject->level->minZoom,
                        mapObject->level->maxZoom,
                        zoom);
                    if (shouldReject)
                        continue;

                    if (!visitor || visitor(mapObject))
                    {
                        if (metric)
                            metric->acceptedMapObjects++;

                        if (resultOut)
                            resultOut->push_back(qMove(mapObject));
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

                if (isExpandedBBox)
                {
                    QList< std::shared_ptr<const BinaryMapObject> > mapObjects;
                    readMapObjectsBlock(
                        reader,
                        section,
                        treeNode,
                        &mapObjects,
                        expandBbox31,
                        filterById != nullptr ? filterReadById : FilterReadingByIdFunction(),
                        visitor,
                        queryController,
                        metric);

                    if (levelSkip)
                    {
                        // select only coastlines if they aren't entering in mapLevel area
                        mapObjects = filterCoastline(mapObjects);
                        if (mapObjects.size() == 0)
                            continue;
                    }

                    for (auto& mapObject : mapObjects)
                    {
                        if (bbox31)
                        {
                            const auto shouldNotSkip =
                                mapObject->bbox31.contains(*bbox31) ||
                                bbox31->intersects(mapObject->bbox31);
                            // no processed if it isn't coastline
                            if (!shouldNotSkip && !isCoastline(mapObject))                            
                                continue;                            
                        }
                        resultOut->push_back(qMove(mapObject));
                    }
                }
                else
                {
                    readMapObjectsBlock(
                        reader,
                        section,
                        treeNode,
                        resultOut,
                        bbox31,
                        filterById != nullptr ? filterReadById : FilterReadingByIdFunction(),
                        visitor,
                        queryController,
                        metric);
                }

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                // Update metric
                if (metric)
                    metric->mapObjectsBlocksRead++;
            }

            // Update metric
            if (metric)
                metric->mapObjectsBlocksProcessed++;
        }

        // Update metric
        if (metric)
            metric->elapsedTimeForMapObjectsBlocks += mapObjectsStopwatch.elapsed();
    }

    // In case cache was supplied, but referenced cache entries output collection was not specified,
    // release all dangling references
    if (cache && !outReferencedCacheEntries)
    {
        for (auto& referencedCacheEntry : danglingReferencedCacheEntries)
            cache->releaseReference(referencedCacheEntry->id, zoom, referencedCacheEntry);
        danglingReferencedCacheEntries.clear();
    }

    if (outBBoxOrSectionSurfaceType)
        *outBBoxOrSectionSurfaceType = bboxOrSectionSurfaceType;

    // In case cache was used, and metric was requested, some values must be taken from different parts
    if (cache && metric)
    {
        metric->elapsedTimeForOnlyAcceptedMapObjects += localMetric.elapsedTimeForOnlyAcceptedMapObjects;
    }
}
