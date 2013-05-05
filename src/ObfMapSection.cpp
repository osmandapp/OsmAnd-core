#include "ObfMapSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

namespace gpb = google::protobuf;

OsmAnd::ObfMapSection::ObfMapSection( class ObfReader* owner )
    : ObfSection(owner)
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

bool OsmAnd::ObfMapSection::isBaseMap()
{
    return QString::compare(_name, "basemap", Qt::CaseInsensitive);
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
            ObfReader::readQString(cis, section->_name);
            break;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                cis->Skip(length);
            }
            break;
        case OBF::OsmAndMapIndex::kLevelsFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<MapLevel> levelRoot(new MapLevel());
                readMapLevel(reader, section, levelRoot.get());
                levelRoot->_length = length;
                levelRoot->_offset = offset;
                section->_levels.push_back(levelRoot);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapLevel( ObfReader* reader, ObfMapSection* section, MapLevel* level )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
            cis->ReadVarint32(&level->_area31.bottom);
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
        case OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_maxZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_minZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
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

void OsmAnd::ObfMapSection::loadMapObjects(
    ObfReader* reader,
    ObfMapSection* section,
    QList< std::shared_ptr<MapObject> >* resultOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    IQueryController* controller /*= nullptr*/)
{
    auto cis = reader->_codedInputStream.get();

    for(auto itLevel = section->_levels.begin(); itLevel != section->_levels.end(); ++itLevel)
    {
        auto level = *itLevel;

        if(filter)
        {
            if(filter->_zoom && (level->_minZoom > *filter->_zoom || level->_maxZoom < *filter->_zoom))
                    continue;
            
            if(filter->_bbox31 && !filter->_bbox31->intersects(level->_area31))
                continue;
        }

        if(level->_trees.isEmpty())
        {
            cis->Seek(level->_offset);
            auto oldLimit = cis->PushLimit(level->_length);
            readLevelTrees(reader, section, level.get(), level->_trees);
            cis->PopLimit(oldLimit);
        }

        QList< std::shared_ptr<LevelTree> > subtrees;
        for(auto itTree = level->_trees.begin(); itTree != level->_trees.end(); ++itTree)
        {
            auto tree = *itTree;

            if(filter && filter->_bbox31 && !filter->_bbox31->intersects(tree->_area31))
                continue;
            
            cis->Seek(tree->_offset);
            auto oldLimit = cis->PushLimit(tree->_length);
            subtrees.push_back(tree);
            loadMapObjects(reader, section, level.get(), tree.get(), subtrees, filter, controller);
            cis->PopLimit(oldLimit);
        }
        qSort(subtrees.begin(), subtrees.end(), [](const std::shared_ptr<LevelTree>& l, const std::shared_ptr<LevelTree>& r) -> int
        {
            return l->_mapDataBlock - r->_mapDataBlock;
        });
        for(auto itTree = subtrees.begin(); itTree != subtrees.end(); ++itTree)
        {
            auto tree = *itTree;
            if(controller && controller->isAborted())
                break;
            
            cis->Seek(tree->_mapDataBlock);
            gpb::uint32 length;
            cis->ReadVarint32(&length);
            auto oldLimit = cis->PushLimit(length);
            readMapObjects(reader, section, tree.get(), resultOut, filter, controller);
            cis->PopLimit(oldLimit);
        }
    }
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

void OsmAnd::ObfMapSection::readLevelTrees( ObfReader* reader, ObfMapSection* section, MapLevel* level, QList< std::shared_ptr<LevelTree> >& trees )
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
                std::shared_ptr<LevelTree> levelTree(new LevelTree());
                levelTree->_isOcean = false;
                levelTree->_offset = offset;
                levelTree->_length = length;
                readLevelTree(reader, section, level, levelTree.get());
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

void OsmAnd::ObfMapSection::readLevelTree( ObfReader* reader, ObfMapSection* section, MapLevel* level, LevelTree* tree )
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
            tree->_area31.left = ObfReader::readSInt32(cis) + level->_area31.left;
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
            tree->_area31.right = ObfReader::readSInt32(cis) + level->_area31.right;
            break;   
        case OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
            tree->_area31.top = ObfReader::readSInt32(cis) + level->_area31.top;
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
            tree->_area31.bottom = ObfReader::readSInt32(cis) + level->_area31.bottom;
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
            tree->_mapDataBlock = ObfReader::readBigEndianInt(cis) + tree->_offset;
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kOceanFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                tree->_isOcean = (value != 0);
            }
            break;
        case OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
            // Pretend that we've never read this tag
            cis->Seek(tagPos);
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::loadMapObjects( ObfReader* reader, ObfMapSection* section, MapLevel* level, LevelTree* tree, QList< std::shared_ptr<LevelTree> >& subtrees, QueryFilter* filter, IQueryController* controller )
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
                std::shared_ptr<LevelTree> subtree(new LevelTree());
                subtree->_isOcean = tree->_isOcean;
                subtree->_offset = offset;
                subtree->_length = length;
                readLevelTree(reader, section, level, subtree.get());
                if(filter && filter->_bbox31 && !filter->_bbox31->intersects(subtree->_area31))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    cis->PopLimit(oldLimit);
                    break;
                }
                subtrees.push_back(subtree);
                loadMapObjects(reader, section, level, subtree.get(), subtrees, filter, controller);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapObjects( ObfReader* reader, ObfMapSection* section, LevelTree* tree, QList< std::shared_ptr<MapObject> >* resultOut, QueryFilter* filter, IQueryController* controller )
{
    auto cis = reader->_codedInputStream.get();

    QList< std::shared_ptr<MapObject> > intermediateResult;
    gpb::uint64 baseId;
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
            cis->ReadVarint64(&baseId);
            break;
        case OBF::MapDataBlock::kDataObjectsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
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

void OsmAnd::ObfMapSection::readMapObject( ObfReader* reader, ObfMapSection* section, LevelTree* tree, std::shared_ptr<OsmAnd::ObfMapSection::MapObject>& mapObject, QueryFilter* filter )
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
                mapObject.reset(new MapObject());
                mapObject->_isArea = (tgn == OBF::MapData::kAreaCoordinatesFieldNumber);
            }
            break;
        case OBF::MapData::kPolygonInnerCoordinatesFieldNumber:
            {
                {
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
            }
            break;
        case OBF::MapData::kAdditionalTypesFieldNumber:
            {
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
            {
                cis->ReadVarint64(&mapObject->_id);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}
