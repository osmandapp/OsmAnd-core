#include "ObfMapSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include <iostream>

#include "OBF.pb.h"

namespace gpb = google::protobuf;

OsmAnd::ObfMapSection::ObfMapSection( class ObfReader* owner_ )
    : ObfSection(owner_)
    , _isBaseMap(false)
    , isBaseMap(_isBaseMap)
    , mapLevels(_mapLevels)
    /*TODO:    , _nameEncodingType(0)
    , _refEncodingType(-1)
    , _coastlineEncodingType(-1)
    , _coastlineBrokenEncodingType(-1)
    , _landEncodingType(-1)
    , _onewayAttribute(-1)
    , _onewayReverseAttribute(-1)*/
{
    /*_positiveLayers.reserve(2);
    _negativeLayers.reserve(2);*/
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
            cis->ReadVarint32(&level->_area31.left);
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
            cis->ReadVarint32(&level->_area31.right);
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
            cis->ReadVarint32(&level->_area31.top);
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
            cis->ReadVarint32(&level->_area31.bottom);
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

void OsmAnd::ObfMapSection::loadRules( ObfReader* reader )
{
    if(!_encodingRules.isEmpty())
        return;

    auto cis = reader->_codedInputStream.get();

    cis->Seek(_offset);
    auto oldLimit = cis->PushLimit(_length);
    readEncodingRules(reader,
        _encodingRules,
        _decodingRules,
        _nameEncodingType,
        _coastlineEncodingType,
        _landEncodingType,
        _onewayAttribute,
        _onewayReverseAttribute,
        _refEncodingType,
        _coastlineBrokenEncodingType,
        _negativeLayers,
        _positiveLayers);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfMapSection::readEncodingRules(
    ObfReader* reader,
    QHash< QString, QHash<QString, uint32_t> >& encodingRules,
    QMap< uint32_t, DecodingRule >& decodingRules,
    uint32_t& nameEncodingType,
    uint32_t& coastlineEncodingType,
    uint32_t& landEncodingType,
    uint32_t& onewayAttribute,
    uint32_t& onewayReverseAttribute,
    uint32_t& refEncodingType,
    uint32_t& coastlineBrokenEncodingType,
    QSet<uint32_t>& negativeLayers,
    QSet<uint32_t>& positiveLayers)
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
                auto free = decodingRules.size() * 2 + 1;
                coastlineBrokenEncodingType = free++;
                {
                    //TODO:initMapEncodingRule(0, _coastlineBrokenEncodingType, "natural", "coastline_broken");
                }
                
                if(landEncodingType == -1)
                {
                    landEncodingType = free++;
                    //TODO:initMapEncodingRule(0, _landEncodingType, "natural", "land");
                }
            }
            return;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readEncodingRule(reader, defaultId++, encodingRules, decodingRules, nameEncodingType, coastlineEncodingType, landEncodingType, onewayAttribute, onewayReverseAttribute, refEncodingType, negativeLayers, positiveLayers);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readEncodingRule(
    ObfReader* reader, uint32_t defaultId,
    QHash< QString, QHash<QString, uint32_t> >& encodingRules,
    QMap< uint32_t, DecodingRule >& decodingRules,
    uint32_t& nameEncodingType,
    uint32_t& coastlineEncodingType,
    uint32_t& landEncodingType,
    uint32_t& onewayAttribute,
    uint32_t& onewayReverseAttribute,
    uint32_t& refEncodingType,
    QSet<uint32_t>& negativeLayers,
    QSet<uint32_t>& positiveLayers)
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
            {
                if(!encodingRules.contains(ruleTag))
                    encodingRules[ruleTag] = QHash<QString, uint32_t>();
                encodingRules[ruleTag][ruleVal] = ruleId;

                if(!decodingRules.contains(ruleId))
                    decodingRules[ruleId] = DecodingRule(ruleTag, ruleVal, ruleType);

                if("name" == ruleTag)
                    nameEncodingType = ruleId;
                else if("natural" == ruleTag && "coastline" == ruleVal)
                    coastlineEncodingType = ruleId;
                else if("natural" == ruleTag && "land" == ruleVal)
                    landEncodingType = ruleId;
                else if("oneway" == ruleTag && "yes" == ruleVal)
                    onewayAttribute = ruleId;
                else if("oneway" == ruleTag && "-1" == ruleVal)
                    onewayReverseAttribute = ruleId;
                else if("ref" == ruleTag)
                    refEncodingType = ruleId;
                else if("tunnel" == ruleTag)
                    negativeLayers.insert(ruleId);
                else if("bridge" == ruleTag)
                    positiveLayers.insert(ruleId);
                else if("layer" == ruleTag)
                {
                    if(!ruleVal.isEmpty() && ruleVal != "0")
                    {
                        if (ruleVal[0] == '-')
                            negativeLayers.insert(ruleId);
                        else
                            positiveLayers.insert(ruleId);
                    }
                }
            }
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

void OsmAnd::ObfMapSection::loadMapObjects(
    ObfReader* reader,
    ObfMapSection* section,
    QList< std::shared_ptr<MapObject> >* resultOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    IQueryController* controller /*= nullptr*/)
{
    auto cis = reader->_codedInputStream.get();

    for(auto itMapLevel = section->_mapLevels.begin(); itMapLevel != section->_mapLevels.end(); ++itMapLevel)
    {
        auto mapLevel = *itMapLevel;

        if(filter)
        {
            if(filter->_zoom && (mapLevel->_minZoom > *filter->_zoom || mapLevel->_maxZoom < *filter->_zoom))
                continue;

            if(filter->_bbox31 && !filter->_bbox31->intersects(mapLevel->_area31))
                continue;
        }

        // If there are no tree nodes in map level, it means they are not loaded
        if(mapLevel->_treeNodes.isEmpty())
        {
            cis->Seek(mapLevel->_offset);
            auto oldLimit = cis->PushLimit(mapLevel->_length);
            readMapLevelTreeNodes(reader, section, mapLevel.get(), mapLevel->_treeNodes);
            cis->PopLimit(oldLimit);
        }

        QList< std::shared_ptr<LevelTreeNode> > treeNodesWithData;
        for(auto itTreeNode = mapLevel->_treeNodes.begin(); itTreeNode != mapLevel->_treeNodes.end(); ++itTreeNode)
        {
            auto treeNode = *itTreeNode;

            if(filter && filter->_bbox31 && !filter->_bbox31->intersects(treeNode->_area31))
                continue;

            if(treeNode->_dataOffset > 0)
                treeNodesWithData.push_back(treeNode);

            cis->Seek(treeNode->_offset);
            auto oldLimit = cis->PushLimit(treeNode->_length);
            readTreeNodeChildren(reader, section, treeNode.get(), &treeNodesWithData, filter, controller);
            assert(cis->BytesUntilLimit() == 0);
            cis->PopLimit(oldLimit);
        }
        qSort(treeNodesWithData.begin(), treeNodesWithData.end(), [](const std::shared_ptr<LevelTreeNode>& l, const std::shared_ptr<LevelTreeNode>& r) -> int
        {
            return l->_dataOffset - r->_dataOffset;
        });
        for(auto itTreeNode = treeNodesWithData.begin(); itTreeNode != treeNodesWithData.end(); ++itTreeNode)
        {
            auto treeNode = *itTreeNode;
            if(controller && controller->isAborted())
                break;

            cis->Seek(treeNode->_dataOffset);
            gpb::uint32 length;
            cis->ReadVarint32(&length);
            auto oldLimit = cis->PushLimit(length);
            readMapObjectsBlock(reader, section, treeNode.get(), resultOut, filter, controller);
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
                levelTree->_isOcean = false;
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
                treeNode->_isOcean = (value != 0);
            }
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readTreeNodeChildren( ObfReader* reader, ObfMapSection* section, LevelTreeNode* treeNode, QList< std::shared_ptr<LevelTreeNode> >* nodesWithData, QueryFilter* filter, IQueryController* controller )
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
                childNode->_isOcean = treeNode->_isOcean;
                childNode->_offset = offset;
                childNode->_length = length;
                readTreeNode(reader, section, treeNode->_area31, childNode.get());
                if(filter && filter->_bbox31 && !filter->_bbox31->intersects(childNode->_area31))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    cis->PopLimit(oldLimit);
                    break;
                }
                
                if(nodesWithData && childNode->_dataOffset > 0)
                    nodesWithData->push_back(childNode);

                cis->Seek(offset);
                readTreeNodeChildren(reader, section, childNode.get(), nodesWithData, filter, controller);
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

void OsmAnd::ObfMapSection::readMapObjectsBlock( ObfReader* reader, ObfMapSection* section, LevelTreeNode* tree, QList< std::shared_ptr<MapObject> >* resultOut, QueryFilter* filter, IQueryController* controller )
{
    auto cis = reader->_codedInputStream.get();

    QList< std::shared_ptr<MapObject> > intermediateResult;
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
                auto entry = *itEntry;
                if(resultOut)
                    resultOut->push_back(entry);
                //TODO: req.publish(obj);
            }
            return;
        case OBF::MapDataBlock::kBaseIdFieldNumber:
            {
                cis->ReadVarint64(&baseId);
            }
            break;
        case OBF::MapDataBlock::kDataObjectsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto pos = cis->CurrentPosition();
                std::shared_ptr<OsmAnd::ObfMapSection::MapObject> mapObject;
                readMapObject(reader, section, tree, mapObject, filter);
                if(mapObject)
                {
                    mapObject->_id += baseId;
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
                QStringList stringTable;
                ObfReader::readStringTable(cis, stringTable);
                for(auto itEntry = intermediateResult.begin(); itEntry != intermediateResult.end(); ++itEntry)
                {
                    auto entry = *itEntry;
                    /*TODO:if (rs.objectNames != null) {
                        int[] keys = rs.objectNames.keys();
                        for (int j = 0; j < keys.length; j++) {
                            rs.objectNames.put(keys[j], stringTable.get(rs.objectNames.get(keys[j]).charAt(0)));
                        }
                    }*/
                }
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapObject( ObfReader* reader, ObfMapSection* section, LevelTreeNode* tree, std::shared_ptr<OsmAnd::ObfMapSection::MapObject>& mapObject, QueryFilter* filter )
{
    auto cis = reader->_codedInputStream.get();

        /*
    List<TIntArrayList> innercoordinates = null;
    TIntArrayList additionalTypes = null;
    TIntObjectHashMap<String> stringNames = null;
    long id = 0;*/
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
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto px = tree->_area31.left & MaskToRead;
                auto py = tree->_area31.top & MaskToRead;
                uint32_t minX = std::numeric_limits<uint32_t>::max();
                uint32_t maxX = 0;
                uint32_t minY = std::numeric_limits<uint32_t>::max();
                uint32_t maxY = 0;
                bool contains = (filter == nullptr);
                //TODO:req.numberOfVisitedObjects++;
                while(cis->BytesUntilLimit() > 0)
                {
                    auto x = (ObfReader::readSInt32(cis) << ShiftCoordinates) + px;
                    auto y = (ObfReader::readSInt32(cis) << ShiftCoordinates) + py;
                    //req.cacheCoordinates.add(x);
                    //req.cacheCoordinates.add(y);
                    px = x;
                    py = y;
                    if(filter && filter->_bbox31 && !contains && filter->_bbox31->contains(x, y))
                        contains = true;
                    if(!contains)
                    {
                        minX = qMin(minX, x);
                        maxX = qMax(maxX, x);
                        minY = qMin(minY, y);
                        maxY = qMax(maxY, y);
                    }
                }
                if(filter && filter->_bbox31 && !contains && filter->_bbox31->contains(maxX, maxY))
                    contains = true;
                cis->PopLimit(oldLimit);
                if(filter && !contains)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }

                // Finally, create the object
                if(!mapObject)
                    mapObject.reset(new MapObject());
                mapObject->_isArea = (tgn == OBF::MapData::kAreaCoordinatesFieldNumber);
            }
            break;
        case OBF::MapData::kPolygonInnerCoordinatesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new MapObject());

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto px = tree->_area31.left & MaskToRead;
                auto py = tree->_area31.top & MaskToRead;
                while(cis->BytesUntilLimit() > 0)
                {
                    auto x = (ObfReader::readSInt32(cis) << ShiftCoordinates) + px;
                    auto y = (ObfReader::readSInt32(cis) << ShiftCoordinates) + py;
                    //TODO:polygon.add(x);
                    //TODO:polygon.add(y);
                    px = x;
                    py = y;
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kAdditionalTypesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new MapObject());

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 type;
                    cis->ReadVarint32(&type);

                    mapObject->_extraTypes.append(type);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kTypesFieldNumber:
            {
                if(!mapObject)
                    mapObject.reset(new MapObject());

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 type;
                    cis->ReadVarint32(&type);

                    mapObject->_types.append(type);
                }
                cis->PopLimit(oldLimit);

                /*
                if (req.searchFilter != null) {
                accept = req.searchFilter.accept(req.cacheTypes, root);
                }
                if (!accept) {
                codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                return null;
                }
                req.numberOfAcceptedObjects++;
                */
            }
            break;
        case OBF::MapData::kStringNamesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 stag;
                    cis->ReadVarint32(&stag);
                    gpb::uint32 sid;
                    cis->ReadVarint32(&sid);

                    mapObject->_names[stag] = sid;
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::MapData::kIdFieldNumber:
            cis->ReadVarint64(&mapObject->_id);
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
    , area31(_area31)
{
}

OsmAnd::ObfMapSection::MapLevel::~MapLevel()
{
}

OsmAnd::ObfMapSection::LevelTreeNode::LevelTreeNode()
    : _dataOffset(0)
{
}
