#include "ObfMapSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include <iostream>

#include "OBF.pb.h"
#include "OsmAndCore/Utilities.h"
#include "MapObject.h"

namespace gpb = google::protobuf;

OsmAnd::ObfMapSection::ObfMapSection( ObfReader* owner_ )
    : ObfSection(owner_)
    , _isBaseMap(false)
    , isBaseMap(_isBaseMap)
    , mapLevels(_mapLevels)
{
}

OsmAnd::ObfMapSection::~ObfMapSection()
{
}

void OsmAnd::ObfMapSection::read( ObfReader* reader, ObfMapSection* section )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex::kNameFieldNumber:
            {
                ObfReader::readQString(cis, section->_name);
                section->_isBaseMap = QString::compare(section->_name, "basemap", Qt::CaseInsensitive);
            }
            break;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            ObfReader::skipUnknownField(cis, tag);
            break;
        case OBF::OsmAndMapIndex::kLevelsFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<MapLevel> levelRoot(new MapLevel());
                readMapLevelHeader(reader, section, levelRoot.get());
                levelRoot->_length = length;
                levelRoot->_offset = offset;
                section->_mapLevels.push_back(levelRoot);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapLevelHeader( ObfReader* reader, ObfMapSection* section, MapLevel* level )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
            cis->ReadVarint32(&level->_maxZoom);
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
            cis->ReadVarint32(&level->_minZoom);
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
            // We skip reading boxes and surely, following blocks
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readRules(
    ObfReader* reader,
    Rules* rules)
{
    auto cis = reader->_codedInputStream.get();

    uint32_t defaultId = 1;
    for(;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            {
                auto free = rules->_decodingRules.size() * 2 + 1;
                rules->_coastlineBrokenEncodingType = free++;
                createRule(rules, 0, rules->_coastlineBrokenEncodingType, "natural", "coastline_broken");
                if(rules->_landEncodingType == -1)
                {
                    rules->_landEncodingType = free++;
                    createRule(rules, 0, rules->_landEncodingType, "natural", "land");
                }
            }
            return;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readRule(reader, defaultId++, rules);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readRule(
    ObfReader* reader,
    uint32_t defaultId,
    Rules* rules)
{
    auto cis = reader->_codedInputStream.get();

    gpb::uint32 ruleId = defaultId;
    gpb::uint32 ruleType = 0;
    QString ruleTag;
    QString ruleVal;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            createRule(rules, ruleType, ruleId, ruleTag, ruleVal);
            return;
        case OBF::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
            ObfReader::readQString(cis, ruleVal);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
            ObfReader::readQString(cis, ruleTag);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTypeFieldNumber:
            cis->ReadVarint32(&ruleType);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kIdFieldNumber:
            cis->ReadVarint32(&ruleId);
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::createRule( Rules* rules, uint32_t ruleType, uint32_t ruleId, const QString& ruleTag, const QString& ruleVal )
{
    auto itEncodingRule = rules->_encodingRules.find(ruleTag);
    if(itEncodingRule == rules->_encodingRules.end())
        itEncodingRule = rules->_encodingRules.insert(ruleTag, QHash<QString, uint32_t>());
    itEncodingRule->insert(ruleVal, ruleId);
    
    if(!rules->_decodingRules.contains(ruleId))
        rules->_decodingRules.insert(ruleId, Rules::DecodingRule(ruleTag, ruleVal, ruleType));

    if("name" == ruleTag)
        rules->_nameEncodingType = ruleId;
    else if("natural" == ruleTag && "coastline" == ruleVal)
        rules->_coastlineEncodingType = ruleId;
    else if("natural" == ruleTag && "land" == ruleVal)
        rules->_landEncodingType = ruleId;
    else if("oneway" == ruleTag && "yes" == ruleVal)
        rules->_onewayAttribute = ruleId;
    else if("oneway" == ruleTag && "-1" == ruleVal)
        rules->_onewayReverseAttribute = ruleId;
    else if("ref" == ruleTag)
        rules->_refEncodingType = ruleId;
    else if("tunnel" == ruleTag)
        rules->_negativeLayers.insert(ruleId);
    else if("bridge" == ruleTag)
        rules->_positiveLayers.insert(ruleId);
    else if("layer" == ruleTag)
    {
        if(!ruleVal.isEmpty() && ruleVal != "0")
        {
            if (ruleVal[0] == '-')
                rules->_negativeLayers.insert(ruleId);
            else
                rules->_positiveLayers.insert(ruleId);
        }
    }
}

void OsmAnd::ObfMapSection::loadMapObjects(
    ObfReader* reader, ObfMapSection* section,
    uint32_t zoom, const AreaI* bbox31 /*= nullptr*/,
    QList< std::shared_ptr<OsmAnd::Model::MapObject> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<OsmAnd::Model::MapObject>&)> visitor /*= nullptr*/,
    IQueryController* controller /*= nullptr*/)
{
    assert(zoom >= 0 && zoom <= 31);
    auto cis = reader->_codedInputStream.get();

    if(!section->_rules)
    {
        cis->Seek(section->_offset);
        auto oldLimit = cis->PushLimit(section->_length);
        section->_rules.reset(new Rules());
        readRules(reader, section->_rules.get());
        cis->PopLimit(oldLimit);
    }

    for(auto itMapLevel = section->_mapLevels.begin(); itMapLevel != section->_mapLevels.end(); ++itMapLevel)
    {
        const auto& mapLevel = *itMapLevel;

        if(mapLevel->_minZoom > zoom || mapLevel->_maxZoom < zoom)
            continue;

        if(bbox31 && !bbox31->intersects(mapLevel->_area31))
            continue;

        // If there are no tree nodes in map level, it means they are not loaded
        QList< std::shared_ptr<LevelTreeNode> > cachedTreeNodes;//TODO: these should be cached somehow
        cis->Seek(mapLevel->_offset);
        auto oldLimit = cis->PushLimit(mapLevel->_length);
        readMapLevelTreeNodes(reader, section, mapLevel.get(), cachedTreeNodes);
        cis->PopLimit(oldLimit);
        
        QList< std::shared_ptr<LevelTreeNode> > treeNodesWithData;
        for(auto itTreeNode = cachedTreeNodes.begin(); itTreeNode != cachedTreeNodes.end(); ++itTreeNode)
        {
            const auto& treeNode = *itTreeNode;

            if(bbox31 && !bbox31->intersects(treeNode->_area31))
                continue;

            if(treeNode->_dataOffset > 0)
                treeNodesWithData.push_back(treeNode);

            cis->Seek(treeNode->_offset);
            auto oldLimit = cis->PushLimit(treeNode->_length);
            readTreeNodeChildren(reader, section, treeNode.get(), &treeNodesWithData, bbox31, controller);
            assert(cis->BytesUntilLimit() == 0);
            cis->PopLimit(oldLimit);
        }
        qSort(treeNodesWithData.begin(), treeNodesWithData.end(), [](const std::shared_ptr<LevelTreeNode>& l, const std::shared_ptr<LevelTreeNode>& r) -> bool
        {
            return l->_dataOffset < r->_dataOffset;
        });
        for(auto itTreeNode = treeNodesWithData.begin(); itTreeNode != treeNodesWithData.end(); ++itTreeNode)
        {
            const auto& treeNode = *itTreeNode;
            if(controller && controller->isAborted())
                break;

            cis->Seek(treeNode->_dataOffset);
            gpb::uint32 length;
            cis->ReadVarint32(&length);
            auto oldLimit = cis->PushLimit(length);
            readMapObjectsBlock(reader, section, treeNode.get(), resultOut, bbox31, visitor, controller);
            cis->PopLimit(oldLimit);
        }
    }
}

void OsmAnd::ObfMapSection::readMapLevelTreeNodes( ObfReader* reader, ObfMapSection* section, MapLevel* level, QList< std::shared_ptr<LevelTreeNode> >& trees )
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<LevelTreeNode> levelTree(new LevelTreeNode());
                levelTree->_offset = offset;
                levelTree->_length = length;

                readTreeNode(reader, section, level->area31, levelTree.get());
                cis->Skip(cis->BytesUntilLimit());

                cis->PopLimit(oldLimit);
                trees.push_back(levelTree);
            }
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readTreeNode( ObfReader* reader, ObfMapSection* section, const AreaI& parentArea, LevelTreeNode* treeNode )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tagPos = cis->CurrentPosition();
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
            {
                auto d = ObfReader::readSInt32(cis);
                treeNode->_area31.left = d + parentArea.left;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
            {
                auto d = ObfReader::readSInt32(cis);
                treeNode->_area31.right = d + parentArea.right;
            }
            break;   
        case OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
            {
                auto d = ObfReader::readSInt32(cis);
                treeNode->_area31.top = d + parentArea.top;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
            {
                auto d = ObfReader::readSInt32(cis);
                treeNode->_area31.bottom = d + parentArea.bottom;
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
            treeNode->_dataOffset = ObfReader::readBigEndianInt(cis) + treeNode->_offset;
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kOceanFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);

                treeNode->_foundation = (value != 0) ? Model::MapObject::FoundationType::FullWater : Model::MapObject::FoundationType::FullLand;
            }
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readTreeNodeChildren(
    ObfReader* reader, ObfMapSection* section,
    LevelTreeNode* treeNode,
    QList< std::shared_ptr<LevelTreeNode> >* nodesWithData,
    const AreaI* bbox31,
    IQueryController* controller)
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<LevelTreeNode> childNode(new LevelTreeNode());
                childNode->_foundation = treeNode->_foundation;
                childNode->_offset = offset;
                childNode->_length = length;
                readTreeNode(reader, section, treeNode->_area31, childNode.get());
                if(bbox31 && !bbox31->intersects(childNode->_area31))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    cis->PopLimit(oldLimit);
                    break;
                }
                
                if(nodesWithData && childNode->_dataOffset > 0)
                    nodesWithData->push_back(childNode);

                cis->Seek(offset);
                readTreeNodeChildren(reader, section, childNode.get(), nodesWithData, bbox31, controller);
                assert(cis->BytesUntilLimit() == 0);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapObjectsBlock(
    ObfReader* reader,
    ObfMapSection* section, 
    LevelTreeNode* tree,
    QList< std::shared_ptr<OsmAnd::Model::MapObject> >* resultOut,
    const AreaI* bbox31,
    std::function<bool (const std::shared_ptr<OsmAnd::Model::MapObject>&)> visitor,
    IQueryController* controller)
{
    auto cis = reader->_codedInputStream.get();

    QList< std::shared_ptr<OsmAnd::Model::MapObject> > intermediateResult;
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
            for(auto itEntry = intermediateResult.begin(); itEntry != intermediateResult.end(); ++itEntry)
            {
                const auto& entry = *itEntry;

                // Fill names of roads from stringtable
                for(auto itNameEntry = entry->_names.begin(); itNameEntry != entry->_names.end(); ++itNameEntry)
                {
                    const auto& encodedId = itNameEntry.value();
                    uint32_t stringId = 0;
                    stringId |= (encodedId.at(1 + 0).unicode() & 0xff) << 8*0;
                    stringId |= (encodedId.at(1 + 1).unicode() & 0xff) << 8*1;
                    stringId |= (encodedId.at(1 + 2).unicode() & 0xff) << 8*2;
                    stringId |= (encodedId.at(1 + 3).unicode() & 0xff) << 8*3;

                    itNameEntry.value() = mapObjectsNamesTable[stringId];
                }

                if(!visitor || visitor(entry))
                {
                    if(resultOut)
                        resultOut->push_back(entry);
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
                auto oldLimit = cis->PushLimit(length);
                auto pos = cis->CurrentPosition();
                std::shared_ptr<OsmAnd::Model::MapObject> mapObject;
                readMapObject(reader, section, tree, baseId, mapObject, bbox31);
                if(mapObject)
                {
                    mapObject->_foundation = tree->_foundation;
                    intermediateResult.push_back(mapObject);
                }
                cis->PopLimit(oldLimit);
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
                ObfReader::readStringTable(cis, mapObjectsNamesTable);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapObject(
    ObfReader* reader,
    ObfMapSection* section,
    LevelTreeNode* treeNode,
    uint64_t baseId,
    std::shared_ptr<OsmAnd::Model::MapObject>& mapObject,
    const AreaI* bbox31)
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        auto tgn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch(tgn)
        {
        case 0:
            return;
        case OBF::MapData::kAreaCoordinatesFieldNumber:
        case OBF::MapData::kCoordinatesFieldNumber:
            {
                QVector< PointI > points31;
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto px = treeNode->_area31.left & MaskToRead;
                auto py = treeNode->_area31.top & MaskToRead;
                int32_t minX = std::numeric_limits<int32_t>::max();
                int32_t maxX = 0;
                int32_t minY = std::numeric_limits<int32_t>::max();
                int32_t maxY = 0;
                bool contains = (bbox31 == nullptr);
                while(cis->BytesUntilLimit() > 0)
                {
                    int32_t dx = (ObfReader::readSInt32(cis) << ShiftCoordinates);
                    auto x = dx + px;
                    int32_t dy = (ObfReader::readSInt32(cis) << ShiftCoordinates);
                    auto y = dy + py;

                    points31.push_back(PointI(x, y));
                    
                    px = x;
                    py = y;
                    if(!contains && bbox31)
                        contains = bbox31->contains(x, y);
                    if(!contains)
                    {
                        minX = qMin(minX, x);
                        maxX = qMax(maxX, x);
                        minY = qMin(minY, y);
                        maxY = qMax(maxY, y);
                    }
                }
                if(!contains && bbox31)
                    contains = bbox31->contains(minX, minY) && bbox31->contains(maxX, maxY);
                cis->PopLimit(oldLimit);
                if(!contains)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }

                // Finally, create the object
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section));
                mapObject->_isArea = (tgn == OBF::MapData::kAreaCoordinatesFieldNumber);
                mapObject->_points31 = points31;
                mapObject->_bbox31.left = minX;
                mapObject->_bbox31.right = maxX;
                mapObject->_bbox31.top = maxY;
                mapObject->_bbox31.bottom = minY;
            }
            break;
        case OBF::MapData::kPolygonInnerCoordinatesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section));

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto px = treeNode->_area31.left & MaskToRead;
                auto py = treeNode->_area31.top & MaskToRead;
                mapObject->_innerPolygonsPoints31.push_back(QVector< PointI >());
                auto& polygon = mapObject->_innerPolygonsPoints31.last();
                while(cis->BytesUntilLimit() > 0)
                {
                    auto dx = (ObfReader::readSInt32(cis) << ShiftCoordinates);
                    auto x = dx + px;
                    auto dy = (ObfReader::readSInt32(cis) << ShiftCoordinates);
                    auto y = dy + py;

                    polygon.push_back(PointI(x, y));
                    
                    px = x;
                    py = y;
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kAdditionalTypesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section));

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 type;
                    cis->ReadVarint32(&type);

                    const auto& tagValue = section->_rules->_decodingRules[type];
                    mapObject->_extraTypes.push_back(TagValue(std::get<0>(tagValue), std::get<1>(tagValue)));
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kTypesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new OsmAnd::Model::MapObject(section));

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 type;
                    cis->ReadVarint32(&type);

                    const auto& tagValue = section->_rules->_decodingRules[type];
                    mapObject->_types.push_back(TagValue(std::get<0>(tagValue), std::get<1>(tagValue)));
                }
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
                    gpb::uint32 stringTag;
                    cis->ReadVarint32(&stringTag);
                    gpb::uint32 stringId;
                    cis->ReadVarint32(&stringId);

                    char fakeString[] = {
                        '[',
                        static_cast<char>((stringId >> 8*0) & 0xff),
                        static_cast<char>((stringId >> 8*1) & 0xff),
                        static_cast<char>((stringId >> 8*2) & 0xff),
                        static_cast<char>((stringId >> 8*3) & 0xff),
                        ']'
                    };
                    const auto& fakeQString = QString::fromLocal8Bit(fakeString, sizeof(fakeString));
                    assert(fakeQString.length() == 6);

                    const auto& tagName = std::get<0>(section->_rules->_decodingRules[stringTag]);
                    mapObject->_names.insert(tagName, fakeQString);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kIdFieldNumber:
            {
                auto d = ObfReader::readSInt64(cis);
                mapObject->_id = d + baseId;
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

OsmAnd::ObfMapSection::MapLevel::MapLevel()
    : minZoom(_minZoom)
    , maxZoom(_maxZoom)
    , length(_length)
    , area31(_area31)
{
}

OsmAnd::ObfMapSection::MapLevel::~MapLevel()
{
}

OsmAnd::ObfMapSection::LevelTreeNode::LevelTreeNode()
    : _dataOffset(0)
    , _foundation(Model::MapObject::FoundationType::Unknown)
{
}

OsmAnd::ObfMapSection::Rules::Rules()
    : _nameEncodingType(0)
    , _refEncodingType(-1)
    , _coastlineEncodingType(-1)
    , _coastlineBrokenEncodingType(-1)
    , _landEncodingType(-1)
    , _onewayAttribute(-1)
    , _onewayReverseAttribute(-1)
{
    _positiveLayers.reserve(2);
    _negativeLayers.reserve(2);
}
