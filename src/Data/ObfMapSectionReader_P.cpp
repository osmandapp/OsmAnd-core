#include "ObfMapSectionReader_P.h"
#include "ObfMapSectionReader.h"
#include "ObfMapSectionReader_Metrics.h"

#include "Common.h"
#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"
#include "ObfReaderUtilities.h"
#include "MapObject.h"
#include "IQueryController.h"
#include "Logging.h"
#include "Utilities.h"

#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>

OsmAnd::ObfMapSectionReader_P::ObfMapSectionReader_P()
{
}

OsmAnd::ObfMapSectionReader_P::~ObfMapSectionReader_P()
{
}

void OsmAnd::ObfMapSectionReader_P::read(
    const ObfReader_P& reader, const std::shared_ptr<ObfMapSectionInfo>& section )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex::kNameFieldNumber:
            {
                ObfReaderUtilities::readQString(cis, section->_name);
                section->_isBasemap = (QString::compare(section->_name, QLatin1String("basemap"), Qt::CaseInsensitive) == 0);
            }
            break;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        case OBF::OsmAndMapIndex::kLevelsFieldNumber:
            {
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfMapSectionLevel> levelRoot(new ObfMapSectionLevel());
                levelRoot->_length = length;
                levelRoot->_offset = offset;
                readMapLevelHeader(reader, section, levelRoot);

                cis->PopLimit(oldLimit);

                section->_levels.push_back(qMove(levelRoot));
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapLevelHeader(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionLevel>& level )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_maxZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_minZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_area31.left));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_area31.right));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_area31.top));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_area31.bottom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                // Save boxes offset
                level->_boxesInnerOffset = tagPos - level->_offset;

                // Skip reading boxes and surely, following blocks
                cis->Skip(cis->BytesUntilLimit());
            }
            return;
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
    auto cis = reader._codedInputStream.get();

    uint32_t defaultId = 1;
    for(;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            {
                encodingDecodingRules->createMissingRules();
            }
            return;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readEncodingDecodingRule(reader, defaultId++, encodingDecodingRules);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readEncodingDecodingRule(
    const ObfReader_P& reader,
    uint32_t defaultId, const std::shared_ptr<ObfMapSectionDecodingEncodingRules>& encodingDecodingRules)
{
    auto cis = reader._codedInputStream.get();

    gpb::uint32 ruleId = defaultId;
    gpb::uint32 ruleType = 0;
    QString ruleTag;
    QString ruleValue;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            encodingDecodingRules->createRule(ruleType, ruleId, ruleTag, ruleValue);
            return;
        case OBF::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
            ObfReaderUtilities::readQString(cis, ruleValue);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
            ObfReaderUtilities::readQString(cis, ruleTag);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTypeFieldNumber:
            cis->ReadVarint32(&ruleType);
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

void OsmAnd::ObfMapSectionReader_P::readMapLevelTreeNodes(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<const ObfMapSectionLevel>& level, QList< std::shared_ptr<ObfMapSectionLevelTreeNode> >& trees )
{
    auto cis = reader._codedInputStream.get();
    for(;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);
                
                std::shared_ptr<ObfMapSectionLevelTreeNode> levelTree(new ObfMapSectionLevelTreeNode(level));
                levelTree->_offset = offset;
                levelTree->_length = length;
                readTreeNode(reader, section, level->area31, levelTree);
                
                cis->PopLimit(oldLimit);

                trees.push_back(qMove(levelTree));
            }
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readTreeNode(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const AreaI& parentArea,
    const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        const auto tagPos = cis->CurrentPosition();
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
            {
                auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->_area31.left = d + parentArea.left;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
            {
                auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->_area31.right = d + parentArea.right;
            }
            break;   
        case OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
            {
                auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->_area31.top = d + parentArea.top;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
            {
                auto d = ObfReaderUtilities::readSInt32(cis);
                treeNode->_area31.bottom = d + parentArea.bottom;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
            treeNode->_dataOffset = ObfReaderUtilities::readBigEndianInt(cis) + treeNode->_offset;
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kOceanFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);

                treeNode->_foundation = (value != 0) ? MapFoundationType::FullWater : MapFoundationType::FullLand;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            {
                // Save children relative offset and skip their data
                treeNode->_childrenInnerOffset = tagPos - treeNode->_offset;
                cis->Skip(cis->BytesUntilLimit());
            }
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readTreeNodeChildren(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode,
    MapFoundationType& foundation,
    QList< std::shared_ptr<ObfMapSectionLevelTreeNode> >* nodesWithData,
    const AreaI* bbox31,
    const IQueryController* const controller,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    auto cis = reader._codedInputStream.get();

    foundation = MapFoundationType::Undefined;

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            {
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<ObfMapSectionLevelTreeNode> childNode(new ObfMapSectionLevelTreeNode(treeNode->level));
                childNode->_foundation = treeNode->_foundation;
                childNode->_offset = offset;
                childNode->_length = length;
                readTreeNode(reader, section, treeNode->_area31, childNode);

                // Update metric
                if(metric)
                    metric->visitedNodes++;

                if(bbox31)
                {
                    const auto shouldSkip =
                        !bbox31->contains(childNode->_area31) &&
                        !childNode->_area31.contains(*bbox31) &&
                        !bbox31->intersects(childNode->_area31);
                    if(shouldSkip)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        cis->PopLimit(oldLimit);
                        break;
                    }
                }
                cis->PopLimit(oldLimit);

                // Update metric
                if(metric)
                    metric->acceptedNodes++;

                if(nodesWithData && childNode->_dataOffset > 0)
                    nodesWithData->push_back(childNode);

                auto childrenFoundation = MapFoundationType::Undefined;
                if(childNode->_childrenInnerOffset > 0)
                {
                    cis->Seek(offset);
                    oldLimit = cis->PushLimit(length);

                    cis->Skip(childNode->_childrenInnerOffset);
                    readTreeNodeChildren(reader, section, childNode, childrenFoundation, nodesWithData, bbox31, controller, metric);
                    assert(cis->BytesUntilLimit() == 0);

                    cis->PopLimit(oldLimit);
                }

                const auto foundationToMerge = (childrenFoundation != MapFoundationType::Undefined) ? childrenFoundation : childNode->_foundation;
                if(foundationToMerge != MapFoundationType::Undefined)
                {
                    if(foundation == MapFoundationType::Undefined)
                        foundation = foundationToMerge;
                    else if(foundation != foundationToMerge)
                        foundation = MapFoundationType::Mixed;
                }
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapObjectsBlock(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionLevelTreeNode>& tree,
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut,
    const AreaI* bbox31,
    const FilterMapObjectsByIdSignature filterById,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor,
    const IQueryController* const controller,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    auto cis = reader._codedInputStream.get();

    QList< std::shared_ptr<Model::MapObject> > intermediateResult;
    QStringList mapObjectsNamesTable;
    gpb::uint64 baseId = 0;
    for(;;)
    {
        if(controller && controller->isAborted())
            return;
        
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            for(const auto& mapObject : constOf(intermediateResult))
            {
                // Fill mapObject names from stringtable
                for(auto& nameValue : mapObject->_names)
                {
                    const auto stringId = ObfReaderUtilities::decodeIntegerFromString(nameValue);

                    if(stringId >= mapObjectsNamesTable.size())
                    {
                        LogPrintf(LogSeverityLevel::Error,
                            "Data mismatch: string #%d (map object #%" PRIu64 " (%" PRIi64 ") not found in string table (size %d) in section '%s'",
                            stringId,
                            mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                            mapObjectsNamesTable.size(), qPrintable(section->name));
                        nameValue = QString::fromLatin1("#%1 NOT FOUND").arg(stringId);
                        continue;
                    }
                    nameValue = mapObjectsNamesTable[stringId];
                }

                if(!visitor || visitor(mapObject))
                {
                    if(resultOut)
                        resultOut->push_back(qMove(mapObject));
                }
            }
            return;
        case OBF::MapDataBlock::kBaseIdFieldNumber:
            cis->ReadVarint64(&baseId);
            break;
        case OBF::MapDataBlock::kDataObjectsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);

                // Update metric
                std::chrono::high_resolution_clock::time_point readMapObject_begin;
                if(metric)
                    readMapObject_begin = std::chrono::high_resolution_clock::now();

                // Read map object content
                std::shared_ptr<OsmAnd::Model::MapObject> mapObject;
                {
                    auto oldLimit = cis->PushLimit(length);

                    readMapObject(reader, section, baseId, tree, mapObject, bbox31);
                    assert(cis->BytesUntilLimit() == 0);

                    // Update metric
                    if(metric)
                        metric->visitedMapObjects++;

                    cis->PopLimit(oldLimit);
                }

                // If map object was not read, skip it
                if(!mapObject)
                {
                    // Update metric
                    if(metric)
                    {
                        const std::chrono::duration<float> readMapObject_elapsed = std::chrono::high_resolution_clock::now() - readMapObject_begin;
                        metric->elapsedTimeForOnlyVisitedMapObjects += readMapObject_elapsed.count();
                    }

                    break;
                }

                // Update metric
                if(metric)
                {
                    const std::chrono::duration<float> readMapObject_elapsed = std::chrono::high_resolution_clock::now() - readMapObject_begin;
                    metric->elapsedTimeForOnlyAcceptedMapObjects += readMapObject_elapsed.count();

                    metric->acceptedMapObjects++;
                }

                // Make unique map object identifier
                mapObject->_id = Model::MapObject::getUniqueId(mapObject->_id, section);

                // Check if map object is desired
                if(filterById && !filterById(section, mapObject->id, mapObject->bbox31, mapObject->level->minZoom, mapObject->level->maxZoom))
                    break;

                // Save object
                mapObject->_foundation = tree->_foundation;
                intermediateResult.push_back(qMove(mapObject));
            }
            break;
        case OBF::MapDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                if(intermediateResult.isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    cis->PopLimit(oldLimit);
                    break;
                }
                ObfReaderUtilities::readStringTable(cis, mapObjectsNamesTable);
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapObject(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    uint64_t baseId, const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode,
    std::shared_ptr<OsmAnd::Model::MapObject>& mapObject,
    const AreaI* bbox31)
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        auto tgn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch(tgn)
        {
        case 0:
            if(mapObject && mapObject->points31.isEmpty())
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Empty MapObject #%" PRIu64 "(%" PRIi64 ") detected in section '%s'",
                    mapObject->id >> 1, static_cast<int64_t>(mapObject->id) / 2,
                    qPrintable(section->name));
                mapObject.reset();
            }
            return;
        case OBF::MapData::kAreaCoordinatesFieldNumber:
        case OBF::MapData::kCoordinatesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                PointI p;
                p.x = treeNode->_area31.left & MaskToRead;
                p.y = treeNode->_area31.top & MaskToRead;

                AreaI objectBBox;
                objectBBox.top = objectBBox.left = std::numeric_limits<int32_t>::max();
                objectBBox.bottom = objectBBox.right = 0;
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
                while(cis->BytesUntilLimit() > 0)
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
                    if(!shouldNotSkip && bbox31)
                    {
                        shouldNotSkip = bbox31->contains(p);

                        objectBBox.enlargeToInclude(p);
                        lastUnprocessedVertexForBBox = verticesCount;
                    }
                }

                cis->PopLimit(oldLimit);

                // Since reserved space may be larger than actual amount of data,
                // shrink the vertices array
                points31.resize(verticesCount);

                // If map object has no vertices, retain it in a special way to report later, when
                // it's identifier will be known
                if(points31.isEmpty())
                {
                    // Fake that this object is inside bbox
                    shouldNotSkip = true;
                    objectBBox = treeNode->_area31;
                }

                // Even if no vertex lays inside bbox, an edge
                // may intersect the bbox
                if(!shouldNotSkip && bbox31)
                {
                    assert(lastUnprocessedVertexForBBox == points31.size());

                    shouldNotSkip =
                        objectBBox.contains(*bbox31) ||
                        bbox31->intersects(objectBBox);
                }

                // If map object didn't fit, skip it's entire content
                if(!shouldNotSkip)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    break;
                }

                // In case bbox is not fully calculated, complete this task
                auto pPointForBBox = points31.data() + lastUnprocessedVertexForBBox;
                while(lastUnprocessedVertexForBBox < points31.size())
                {
                    objectBBox.enlargeToInclude(*pPointForBBox);

                    lastUnprocessedVertexForBBox++;
                    pPointForBBox++;
                }

                // Finally, create the object
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section, treeNode->level));
                mapObject->_isArea = (tgn == OBF::MapData::kAreaCoordinatesFieldNumber);
                mapObject->_points31 = qMove(points31);
                mapObject->_bbox31 = objectBBox;
                assert(treeNode->_area31.top - mapObject->_bbox31.top <= 32);
                assert(treeNode->_area31.left - mapObject->_bbox31.left <= 32);
                assert(mapObject->_bbox31.bottom - treeNode->_area31.bottom <= 1);
                assert(mapObject->_bbox31.right - treeNode->_area31.right <= 1);
                assert(mapObject->_bbox31.right >= mapObject->_bbox31.left);
                assert(mapObject->_bbox31.bottom >= mapObject->_bbox31.top);
            }
            break;
        case OBF::MapData::kPolygonInnerCoordinatesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section, treeNode->level));

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                PointI p;
                p.x = treeNode->_area31.left & MaskToRead;
                p.y = treeNode->_area31.top & MaskToRead;

                // Preallocate memory
                const auto probableVerticesCount = (cis->BytesUntilLimit() / 2);
                mapObject->_innerPolygonsPoints31.push_back(qMove(QVector< PointI >(probableVerticesCount)));
                auto& polygon = mapObject->_innerPolygonsPoints31.last();

                auto pPoint = polygon.data();
                auto verticesCount = 0;
                while(cis->BytesUntilLimit() > 0)
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
            }
            break;
        case OBF::MapData::kAdditionalTypesFieldNumber:
        case OBF::MapData::kTypesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section, treeNode->level));

                auto& typesRuleIds = (tgn == OBF::MapData::kAdditionalTypesFieldNumber ? mapObject->_extraTypesRuleIds : mapObject->_typesRuleIds);

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                // Preallocate space
                typesRuleIds.reserve(cis->BytesUntilLimit());

                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 ruleId;
                    cis->ReadVarint32(&ruleId);

                    typesRuleIds.push_back(ruleId);
                }

                // Shrink preallocated space
                typesRuleIds.squeeze();

                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kStringNamesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    bool ok;

                    gpb::uint32 stringRuleId;
                    ok = cis->ReadVarint32(&stringRuleId);
                    assert(ok);
                    gpb::uint32 stringId;
                    ok = cis->ReadVarint32(&stringId);
                    assert(ok);

                    mapObject->_names.insert(stringRuleId, qMove(ObfReaderUtilities::encodeIntegerToString(stringId)));
                }
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kIdFieldNumber:
            {
                auto d = ObfReaderUtilities::readSInt64(cis);

                mapObject->_id = d + baseId;
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::loadMapObjects(
    const ObfReader_P& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    ZoomLevel zoom, const AreaI* bbox31,
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut, MapFoundationType* foundationOut,
    const FilterMapObjectsByIdSignature filterById,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor,
    const IQueryController* const controller,
    ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric)
{
    auto cis = reader._codedInputStream.get();

    // Check if this map section has initialized rules
    {
        QMutexLocker scopedLock(&section->_p->_encodingDecodingDataMutex);

        if(!section->_p->_encodingDecodingRules)
        {
            cis->Seek(section->_offset);
            auto oldLimit = cis->PushLimit(section->_length);

            std::shared_ptr<ObfMapSectionDecodingEncodingRules> encodingDecodingRules(new ObfMapSectionDecodingEncodingRules());
            readEncodingDecodingRules(reader, encodingDecodingRules);
            section->_p->_encodingDecodingRules = encodingDecodingRules;

            cis->PopLimit(oldLimit);
        }
    }
    
    auto foundation = MapFoundationType::Undefined;
    if(foundationOut)
        foundation = *foundationOut;
    for(const auto& mapLevel : constOf(section->_levels))
    {
        // Update metric
        if(metric)
            metric->visitedLevels++;

        if(mapLevel->_minZoom > zoom || mapLevel->_maxZoom < zoom)
            continue;

        if(bbox31)
        {
            const auto shouldSkip =
                !bbox31->contains(mapLevel->_area31) &&
                !mapLevel->_area31.contains(*bbox31) &&
                !bbox31->intersects(mapLevel->_area31);
            if(shouldSkip)
                continue;
        }

        // Update metric
        std::chrono::high_resolution_clock::time_point treeNodes_begin;
        if(metric)
        {
            metric->acceptedLevels++;
            treeNodes_begin = std::chrono::high_resolution_clock::now();
        }

        // If there are no tree nodes in map level, it means they are not loaded
        {
            QMutexLocker scopedLock(&mapLevel->_p->_rootNodesMutex);

            if(!mapLevel->_p->_rootNodes)
            {
                cis->Seek(mapLevel->_offset);
                auto oldLimit = cis->PushLimit(mapLevel->_length);
                
                cis->Skip(mapLevel->_boxesInnerOffset);
                mapLevel->_p->_rootNodes.reset(new ObfMapSectionLevel_P::RootNodes());
                readMapLevelTreeNodes(reader, section, mapLevel, mapLevel->_p->_rootNodes->nodes);

                cis->PopLimit(oldLimit);
            }
        }
        
        // Collect tree nodes with data
        QList< std::shared_ptr<ObfMapSectionLevelTreeNode> > treeNodesWithData;
        for(const auto& rootNode : constOf(mapLevel->_p->_rootNodes->nodes))
        {
            // Update metric
            if(metric)
                metric->visitedNodes++;

            if(bbox31)
            {
                const auto shouldSkip =
                    !bbox31->contains(rootNode->_area31) &&
                    !rootNode->_area31.contains(*bbox31) &&
                    !bbox31->intersects(rootNode->_area31);
                if(shouldSkip)
                    continue;
            }

            // Update metric
            if(metric)
                metric->acceptedNodes++;

            if(rootNode->_dataOffset > 0)
                treeNodesWithData.push_back(rootNode);

            auto childrenFoundation = MapFoundationType::Undefined;
            if(rootNode->_childrenInnerOffset > 0)
            {
                cis->Seek(rootNode->_offset);
                auto oldLimit = cis->PushLimit(rootNode->_length);

                cis->Skip(rootNode->_childrenInnerOffset);
                readTreeNodeChildren(reader, section, rootNode, childrenFoundation, &treeNodesWithData, bbox31, controller, metric);
                assert(cis->BytesUntilLimit() == 0);

                cis->PopLimit(oldLimit);
            }

            const auto foundationToMerge = (childrenFoundation != MapFoundationType::Undefined) ? childrenFoundation : rootNode->_foundation;
            if(foundationToMerge != MapFoundationType::Undefined)
            {
                if(foundation == MapFoundationType::Undefined)
                    foundation = foundationToMerge;
                else if(foundation != foundationToMerge)
                    foundation = MapFoundationType::Mixed;
            }
        }

        // Sort blocks by data offset to force forward-only seeking
        qSort(treeNodesWithData.begin(), treeNodesWithData.end(), [](const std::shared_ptr<ObfMapSectionLevelTreeNode>& l, const std::shared_ptr<ObfMapSectionLevelTreeNode>& r) -> bool
        {
            return l->_dataOffset < r->_dataOffset;
        });

        // Update metric
        std::chrono::high_resolution_clock::time_point mapObjects_begin;
        if(metric)
        {
            const std::chrono::duration<float> treeNodes_elapsed = std::chrono::high_resolution_clock::now() - treeNodes_begin;
            metric->elapsedTimeForNodes += treeNodes_elapsed.count();

            mapObjects_begin = std::chrono::high_resolution_clock::now();
        }

        // Read map objects from their blocks
        for(const auto& treeNode : constOf(treeNodesWithData))
        {
            if(controller && controller->isAborted())
                break;

            cis->Seek(treeNode->_dataOffset);

            gpb::uint32 length;
            cis->ReadVarint32(&length);
            auto oldLimit = cis->PushLimit(length);

            readMapObjectsBlock(reader, section, treeNode, resultOut, bbox31, filterById, visitor, controller, metric);
            assert(cis->BytesUntilLimit() == 0);

            cis->PopLimit(oldLimit);

            // Update metric
            if(metric)
                metric->mapObjectsBlocksRead++;
        }

        // Update metric
        if(metric)
        {
            const std::chrono::duration<float> mapObjects_elapsed = std::chrono::high_resolution_clock::now() - mapObjects_begin;
            metric->elapsedTimeForMapObjectsBlocks += mapObjects_elapsed.count();
        }
    }

    if(foundationOut)
        *foundationOut = foundation;
}
