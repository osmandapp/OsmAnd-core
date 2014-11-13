#include "ObfMapSectionReader_P.h"
#include "ObfMapSectionReader.h"
#include "ObfMapSectionReader_Metrics.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
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
    const auto cis = reader._codedInputStream.get();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
            case OBF::OsmAndMapIndex::kNameFieldNumber:
            {
                ObfReaderUtilities::readQString(cis, section->_name);
                section->isBasemap = (QString::compare(section->_name, QLatin1String("basemap"), Qt::CaseInsensitive) == 0);
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
                readMapLevelHeader(reader, section, levelRoot);

                assert(cis->BytesUntilLimit() == 0);
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

void OsmAnd::ObfMapSectionReader_P::readEncodingDecodingRules(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfMapSectionDecodingEncodingRules>& encodingDecodingRules)
{
    const auto cis = reader._codedInputStream.get();

    uint32_t defaultId = 1;
    for (;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                encodingDecodingRules->verifyRequiredRulesExist();
                return;
            case OBF::OsmAndMapIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                readEncodingDecodingRule(reader, defaultId++, encodingDecodingRules);

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

void OsmAnd::ObfMapSectionReader_P::readEncodingDecodingRule(
    const ObfReader_P& reader,
    uint32_t defaultId,
    const std::shared_ptr<ObfMapSectionDecodingEncodingRules>& encodingDecodingRules)
{
    const auto cis = reader._codedInputStream.get();

    gpb::uint32 ruleId = defaultId;
    QString ruleTag;
    QString ruleValue;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                encodingDecodingRules->addRule(ruleId, ruleTag, ruleValue);
                return;
            case OBF::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
                ObfReaderUtilities::readQString(cis, ruleValue);
                break;
            case OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
                ObfReaderUtilities::readQString(cis, ruleTag);
                break;
            case OBF::OsmAndMapIndex_MapEncodingRule::kIdFieldNumber:
                cis->ReadVarint32(&ruleId);
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapLevelHeader(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionLevel>& level)
{
    const auto cis = reader._codedInputStream.get();

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
    const auto cis = reader._codedInputStream.get();
    for (;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
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

                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

                trees.push_back(qMove(levelTree));

                break;
            }
            default:
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
    const auto cis = reader._codedInputStream.get();

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

                treeNode->foundation = (value != 0) ? MapFoundationType::FullWater : MapFoundationType::FullLand;
                assert(
                    (treeNode->foundation != MapFoundationType::FullWater) ||
                    (treeNode->foundation == MapFoundationType::FullWater && section->isBasemap));

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
    MapFoundationType& outChildrenFoundation,
    QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >* nodesWithData,
    const AreaI* bbox31,
    const IQueryController* const controller,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader._codedInputStream.get();

    outChildrenFoundation = MapFoundationType::Undefined;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
            case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfMapSectionLevelTreeNode> childNode(new ObfMapSectionLevelTreeNode(treeNode->level));
                childNode->foundation = treeNode->foundation;
                childNode->offset = offset;
                childNode->length = length;
                readTreeNode(reader, section, treeNode->area31, childNode);

                assert(cis->BytesUntilLimit() == 0);
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

                auto subchildrenFoundation = MapFoundationType::Undefined;
                if (childNode->hasChildrenDataBoxes)
                {
                    cis->Seek(childNode->offset);
                    const auto oldLimit = cis->PushLimit(length);

                    cis->Skip(childNode->firstDataBoxInnerOffset);
                    readTreeNodeChildren(reader, section, childNode, subchildrenFoundation, nodesWithData, bbox31, controller, metric);
                    
                    if (const auto bytesUntilLimit = cis->BytesUntilLimit())
                        LogPrintf(LogSeverityLevel::Error, "Error in reader: %d bytes until limit", bytesUntilLimit);
                    assert(cis->BytesUntilLimit() == 0);
                    cis->PopLimit(oldLimit);
                }

                const auto foundationToMerge = (subchildrenFoundation != MapFoundationType::Undefined) ? subchildrenFoundation : childNode->foundation;
                if (foundationToMerge != MapFoundationType::Undefined)
                {
                    if (outChildrenFoundation == MapFoundationType::Undefined)
                        outChildrenFoundation = foundationToMerge;
                    else if (outChildrenFoundation != foundationToMerge)
                        outChildrenFoundation = MapFoundationType::Mixed;
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
    const FilterMapObjectsByIdFunction filterById,
    const VisitorFunction visitor,
    const IQueryController* const controller,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader._codedInputStream.get();

    QList< std::shared_ptr<BinaryMapObject> > intermediateResult;
    QStringList mapObjectsCaptionsTable;
    gpb::uint64 baseId = 0;
    for (;;)
    {
        if (controller && controller->isAborted())
            return;

        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
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
                    //if ((mapObject->id >> 1) == 7374044u)
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

                // Read map object content
                const Stopwatch readMapObjectStopwatch(metric != nullptr);
                std::shared_ptr<OsmAnd::BinaryMapObject> mapObject;
                auto oldLimit = cis->PushLimit(length);
                
                readMapObject(reader, section, baseId, tree, mapObject, bbox31, metric);

                assert(cis->BytesUntilLimit() == 0);
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
                //if ((mapObject->id >> 1) == 7374044u)
                //{
                //    int i = 5;
                //}
                //////////////////////////////////////////////////////////////////////////

                // Check if map object is desired
                if (filterById && !filterById(section, mapObject->id, mapObject->bbox31, mapObject->level->minZoom, mapObject->level->maxZoom))
                    break;

                // Save object
                intermediateResult.push_back(qMove(mapObject));

                break;
            }
            case OBF::MapDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                if (intermediateResult.isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());

                    assert(cis->BytesUntilLimit() == 0);
                    cis->PopLimit(oldLimit);
                    break;
                }
                
                ObfReaderUtilities::readStringTable(cis, mapObjectsCaptionsTable);

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

void OsmAnd::ObfMapSectionReader_P::readMapObject(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    uint64_t baseId,
    const std::shared_ptr<const ObfMapSectionLevelTreeNode>& treeNode,
    std::shared_ptr<OsmAnd::BinaryMapObject>& mapObject,
    const AreaI* bbox31,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader._codedInputStream.get();
    const auto baseOffset = cis->CurrentPosition();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        const auto tgn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch (tgn)
        {
            case 0:
            {
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

                assert(cis->BytesUntilLimit() == 0);
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

                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

                break;
            }
            case OBF::MapData::kAdditionalTypesFieldNumber:
            case OBF::MapData::kTypesFieldNumber:
            {
                if (!mapObject)
                    mapObject.reset(new OsmAnd::BinaryMapObject(section, treeNode->level));

                auto& typesRuleIds = (tgn == OBF::MapData::kAdditionalTypesFieldNumber)
                    ? mapObject->additionalTypesRuleIds
                    : mapObject->typesRuleIds;

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                // Preallocate space
                typesRuleIds.reserve(cis->BytesUntilLimit());

                while (cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 ruleId;
                    cis->ReadVarint32(&ruleId);

                    typesRuleIds.push_back(ruleId);
                }

                // Shrink preallocated space
                typesRuleIds.squeeze();

                assert(cis->BytesUntilLimit() == 0);
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

                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

                break;
            }
            case OBF::MapData::kIdFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt64(cis);
                const auto rawId = static_cast<uint64_t>(d + baseId);
                mapObject->id = ObfObjectId::generateUniqueId(rawId, baseOffset, section);

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::loadMapObjects(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    ZoomLevel zoom,
    const AreaI* bbox31,
    QList< std::shared_ptr<const OsmAnd::BinaryMapObject> >* resultOut,
    MapFoundationType* outBBoxOrSectionFoundation,
    const FilterMapObjectsByIdFunction filterById,
    const VisitorFunction visitor,
    DataBlocksCache* cache,
    QList< std::shared_ptr<const DataBlock> >* outReferencedCacheEntries,
    const IQueryController* const controller,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    const auto cis = reader._codedInputStream.get();

    // Ensure encoding/decoding rules are read
#if defined(ATOMIC_POINTER_LOCK_FREE)
    if (std::atomic_load(&section->_p->_encodingDecodingRules) == nullptr)
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
    if (section->_p->_encodingDecodingRulesLoaded.loadAcquire() == 0)
#endif
    {
        QMutexLocker scopedLocker(&section->_p->_encodingDecodingRulesLoadMutex);
#if defined(ATOMIC_POINTER_LOCK_FREE)
        if (std::atomic_load(&section->_p->_encodingDecodingRules) == nullptr)
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
        if (!section->_p->_encodingDecodingRules)
#endif
        {
            // Read encoding/decoding rules
            cis->Seek(section->_offset);
            auto oldLimit = cis->PushLimit(section->_length);

            const std::shared_ptr<ObfMapSectionDecodingEncodingRules> encodingDecodingRules(new ObfMapSectionDecodingEncodingRules());
            readEncodingDecodingRules(reader, encodingDecodingRules);
#if defined(ATOMIC_POINTER_LOCK_FREE)
            std::atomic_store(&section->_p->_encodingDecodingRules, encodingDecodingRules);
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
            section->_p->_encodingDecodingRules = encodingDecodingRules;
#endif

            assert(cis->BytesUntilLimit() == 0);
            cis->PopLimit(oldLimit);

#if !defined(ATOMIC_POINTER_LOCK_FREE)
            section->_p->_encodingDecodingRulesLoaded.storeRelease(1);
#endif // !defined(ATOMIC_POINTER_LOCK_FREE)
        }
    }

    ObfMapSectionReader_Metrics::Metric_loadMapObjects localMetric;

    auto bboxOrSectionFoundation = MapFoundationType::Undefined;
    if (outBBoxOrSectionFoundation)
        *outBBoxOrSectionFoundation = bboxOrSectionFoundation;
    QList< std::shared_ptr<const DataBlock> > danglingReferencedCacheEntries;
    for (const auto& mapLevel : constOf(section->levels))
    {
        // Update metric
        if (metric)
            metric->visitedLevels++;

        if (mapLevel->minZoom > zoom || mapLevel->maxZoom < zoom)
            continue;

        if (bbox31)
        {
            const Stopwatch bboxLevelCheckStopwatch(metric != nullptr);

            const auto shouldSkip =
                !bbox31->contains(mapLevel->area31) &&
                !mapLevel->area31.contains(*bbox31) &&
                !bbox31->intersects(mapLevel->area31);

            if (metric)
                metric->elapsedTimeForLevelsBbox += bboxLevelCheckStopwatch.elapsed();

            if (shouldSkip)
                continue;
        }

        const Stopwatch treeNodesStopwatch(metric != nullptr);
        if (metric)
            metric->acceptedLevels++;

        // If there are no tree nodes in map level, it means they are not loaded.
        // Since loading may be called from multiple threads, loading of root nodes needs synchronization
        // Ensure encoding/decoding rules are read
#if defined(ATOMIC_POINTER_LOCK_FREE)
        if (std::atomic_load(&mapLevel->_p->_rootNodes) == nullptr)
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
        if (mapLevel->_p->_rootNodesLoaded.loadAcquire() == 0)
#endif
        {
            QMutexLocker scopedLocker(&mapLevel->_p->_rootNodesLoadMutex);
#if defined(ATOMIC_POINTER_LOCK_FREE)
            if (std::atomic_load(&mapLevel->_p->_rootNodes) == nullptr)
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
            if (!mapLevel->_p->_rootNodes)
#endif
            {
                cis->Seek(mapLevel->offset);
                auto oldLimit = cis->PushLimit(mapLevel->length);

                cis->Skip(mapLevel->firstDataBoxInnerOffset);
                const std::shared_ptr< QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> > > rootNodes(
                    new QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> >());
                readMapLevelTreeNodes(reader, section, mapLevel, *rootNodes);
#if defined(ATOMIC_POINTER_LOCK_FREE)
                std::atomic_store(&mapLevel->_p->_rootNodes, std::static_pointer_cast<const QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> > >(rootNodes));
#else // !defined(ATOMIC_POINTER_LOCK_FREE)
                mapLevel->_p->_rootNodes = rootNodes;
#endif

                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);

#if !defined(ATOMIC_POINTER_LOCK_FREE)
                mapLevel->_p->_rootNodesLoaded.storeRelease(1);
#endif // !defined(ATOMIC_POINTER_LOCK_FREE)
            }
        }

        // Collect tree nodes with data
        QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> > treeNodesWithData;
        for (const auto& rootNode : constOf(*mapLevel->_p->_rootNodes))
        {
            // Update metric
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

            auto rootSubnodesFoundation = MapFoundationType::Undefined;
            if (rootNode->hasChildrenDataBoxes)
            {
                cis->Seek(rootNode->offset);
                auto oldLimit = cis->PushLimit(rootNode->length);

                cis->Skip(rootNode->firstDataBoxInnerOffset);
                readTreeNodeChildren(reader, section, rootNode, rootSubnodesFoundation, &treeNodesWithData, bbox31, controller, metric);
                
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);
            }

            const auto foundationToMerge = (rootSubnodesFoundation != MapFoundationType::Undefined) ? rootSubnodesFoundation : rootNode->foundation;
            if (foundationToMerge != MapFoundationType::Undefined)
            {
                if (bboxOrSectionFoundation == MapFoundationType::Undefined)
                    bboxOrSectionFoundation = foundationToMerge;
                else if (bboxOrSectionFoundation != foundationToMerge)
                    bboxOrSectionFoundation = MapFoundationType::Mixed;
            }
        }

        // Sort blocks by data offset to force forward-only seeking
        qSort(treeNodesWithData.begin(), treeNodesWithData.end(),
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
            if (controller && controller->isAborted())
                break;

            DataBlockId blockId;
            blockId.sectionRuntimeGeneratedId = section->runtimeGeneratedId;
            blockId.offset = treeNode->dataOffset;

            if (cache && cache->shouldCacheBlock(blockId, treeNode->area31, bbox31))
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

                    assert(cis->BytesUntilLimit() == 0);
                    cis->PopLimit(oldLimit);

                    // Update metric
                    if (metric)
                        metric->mapObjectsBlocksRead++;

                    // Create a data block and share it
                    dataBlock.reset(new DataBlock(blockId, treeNode->area31, treeNode->foundation, mapObjects));
                    cache->fulfilPromiseAndReference(blockId, levelZooms, dataBlock);
                }

                if (outReferencedCacheEntries)
                    outReferencedCacheEntries->push_back(dataBlock);
                else
                    danglingReferencedCacheEntries.push_back(dataBlock);

                // Process data block
                for (const auto& mapObject : constOf(dataBlock->mapObjects))
                {
                    if (metric)
                        metric->visitedMapObjects++;

                    if (bbox31)
                    {
                        const auto shouldNotSkip =
                            mapObject->bbox31.contains(*bbox31) ||
                            bbox31->intersects(mapObject->bbox31);

                        if (!shouldNotSkip)
                            continue;
                    }

                    // Check if map object is desired
                    if (filterById && !filterById(section, mapObject->id, mapObject->bbox31, mapObject->level->minZoom, mapObject->level->maxZoom))
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

                readMapObjectsBlock(
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

    // In case cache was supplied, but referenced cache entries output collection was not specified, release all dangling references
    if (cache && !outReferencedCacheEntries)
    {
        for (auto& referencedCacheEntry : danglingReferencedCacheEntries)
            cache->releaseReference(referencedCacheEntry->id, zoom, referencedCacheEntry);
        danglingReferencedCacheEntries.clear();
    }

    if (outBBoxOrSectionFoundation)
        *outBBoxOrSectionFoundation = bboxOrSectionFoundation;

    // In case cache was used, and metric was requested, some values must be taken from different parts
    if (cache && metric)
    {
        metric->elapsedTimeForOnlyAcceptedMapObjects += localMetric.elapsedTimeForOnlyAcceptedMapObjects;
    }
}
