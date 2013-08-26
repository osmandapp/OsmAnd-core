#include "ObfMapSectionReader_P.h"

#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionInfo_P.h"
#include "ObfReaderUtilities.h"
#include "MapObject.h"
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
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfMapSectionInfo>& section )
{
    auto cis = reader->_codedInputStream.get();

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
                section->_isBasemap = (QString::compare(section->_name, QString::fromLatin1("basemap"), Qt::CaseInsensitive) == 0);
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
                std::shared_ptr<ObfMapSectionLevel> levelRoot(new ObfMapSectionLevel());
                levelRoot->_length = length;
                levelRoot->_offset = offset;
                readMapLevelHeader(reader, section, levelRoot);
                section->_levels.push_back(levelRoot);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapLevelHeader(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionLevel>& level )
{
    auto cis = reader->_codedInputStream.get();

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

void OsmAnd::ObfMapSectionReader_P::readRules(
    const std::unique_ptr<ObfReader_P>& reader,
    const std::shared_ptr<ObfMapSectionInfo_P::Rules>& rules)
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readRule(
    const std::unique_ptr<ObfReader_P>& reader,
    uint32_t defaultId, const std::shared_ptr<ObfMapSectionInfo_P::Rules>& rules)
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
            ObfReaderUtilities::readQString(cis, ruleVal);
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

void OsmAnd::ObfMapSectionReader_P::createRule( const std::shared_ptr<ObfMapSectionInfo_P::Rules>& rules, uint32_t ruleType, uint32_t ruleId, const QString& ruleTag, const QString& ruleVal )
{
    auto itEncodingRule = rules->_encodingRules.find(ruleTag);
    if(itEncodingRule == rules->_encodingRules.end())
        itEncodingRule = rules->_encodingRules.insert(ruleTag, QHash<QString, uint32_t>());
    itEncodingRule->insert(ruleVal, ruleId);
    
    if(!rules->_decodingRules.contains(ruleId))
        rules->_decodingRules.insert(ruleId, ObfMapSectionInfo_P::Rules::DecodingRule(ruleTag, ruleVal, ruleType));

    if(QString::fromLatin1("name") == ruleTag)
        rules->_nameEncodingType = ruleId;
    else if(QString::fromLatin1("natural") == ruleTag && QString::fromLatin1("coastline") == ruleVal)
        rules->_coastlineEncodingType = ruleId;
    else if(QString::fromLatin1("natural") == ruleTag && QString::fromLatin1("land") == ruleVal)
        rules->_landEncodingType = ruleId;
    else if(QString::fromLatin1("oneway") == ruleTag && QString::fromLatin1("yes") == ruleVal)
        rules->_onewayAttribute = ruleId;
    else if(QString::fromLatin1("oneway") == ruleTag && QString::fromLatin1("-1") == ruleVal)
        rules->_onewayReverseAttribute = ruleId;
    else if(QString::fromLatin1("ref") == ruleTag)
        rules->_refEncodingType = ruleId;
    else if(QString::fromLatin1("tunnel") == ruleTag)
        rules->_negativeLayers.insert(ruleId);
    else if(QString::fromLatin1("bridge") == ruleTag)
        rules->_positiveLayers.insert(ruleId);
    else if(QString::fromLatin1("layer") == ruleTag)
    {
        if(!ruleVal.isEmpty() && ruleVal != QString::fromLatin1("0"))
        {
            if (ruleVal[0] == '-')
                rules->_negativeLayers.insert(ruleId);
            else
                rules->_positiveLayers.insert(ruleId);
        }
    }
}

void OsmAnd::ObfMapSectionReader_P::readMapLevelTreeNodes(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<const ObfMapSectionLevel>& level, QList< std::shared_ptr<ObfMapSectionLevelTreeNode> >& trees )
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
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<ObfMapSectionLevelTreeNode> levelTree(new ObfMapSectionLevelTreeNode());
                levelTree->_offset = offset;
                levelTree->_length = length;

                readTreeNode(reader, section, level->area31, levelTree);
                
                cis->PopLimit(oldLimit);
                trees.push_back(levelTree);
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
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const AreaI& parentArea, const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode )
{
    auto cis = reader->_codedInputStream.get();

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
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<ObfMapSectionLevelTreeNode>& treeNode,
    QList< std::shared_ptr<ObfMapSectionLevelTreeNode> >* nodesWithData,
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
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<ObfMapSectionLevelTreeNode> childNode(new ObfMapSectionLevelTreeNode());
                childNode->_foundation = treeNode->_foundation;
                childNode->_offset = offset;
                childNode->_length = length;
                readTreeNode(reader, section, treeNode->_area31, childNode);
                if(bbox31 && !bbox31->intersects(childNode->_area31))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    cis->PopLimit(oldLimit);
                    break;
                }
                cis->PopLimit(oldLimit);
                
                if(nodesWithData && childNode->_dataOffset > 0)
                    nodesWithData->push_back(childNode);

                cis->Seek(offset);
                oldLimit = cis->PushLimit(length);
                cis->Skip(childNode->_childrenInnerOffset);
                readTreeNodeChildren(reader, section, childNode, nodesWithData, bbox31, controller);
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

void OsmAnd::ObfMapSectionReader_P::readMapObjectsBlock(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    ObfMapSectionLevelTreeNode* tree,
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut,
    const AreaI* bbox31,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor,
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
                ObfReaderUtilities::readStringTable(cis, mapObjectsNamesTable);
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
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    ObfMapSectionLevelTreeNode* treeNode,
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
                auto minX = std::numeric_limits<int32_t>::max();
                auto maxX = 0;
                auto minY = std::numeric_limits<int32_t>::max();
                auto maxY = 0;
                bool contains = (bbox31 == nullptr);
                while(cis->BytesUntilLimit() > 0)
                {
                    //NOTE: Here was 'auto', but Apple LLVM didn't like it
                    int32_t dx = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
                    int32_t x = dx + px;
                    int32_t dy = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
                    int32_t y = dy + py;

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
                    auto dx = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
                    auto x = dx + px;
                    auto dy = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
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

                    const auto& tagValue = section->_d->_rules->_decodingRules[type];
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

                    const auto& tagValue = section->_d->_rules->_decodingRules[type];
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

                    const auto& tagName = std::get<0>(section->_d->_rules->_decodingRules[stringTag]);
                    mapObject->_names.insert(tagName, fakeQString);
                }
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
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
    ZoomLevel zoom, const AreaI* bbox31 /*= nullptr*/,
    QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor /*= nullptr*/,
    IQueryController* controller /*= nullptr*/)
{
    auto cis = reader->_codedInputStream.get();

    // Check if this map section has initialized rules
    {
        QMutexLocker scopedLock(&section->_d->_rulesMutex);

        if(!section->_d->_rules)
        {
            cis->Seek(section->_offset);
            auto oldLimit = cis->PushLimit(section->_length);
            section->_d->_rules.reset(new ObfMapSectionInfo_P::Rules());
            readRules(reader, section->_d->_rules);
            cis->PopLimit(oldLimit);
        }
    }
    
    for(auto itMapLevel = section->_levels.begin(); itMapLevel != section->_levels.end(); ++itMapLevel)
    {
        const auto& mapLevel = *itMapLevel;

        if(mapLevel->_minZoom > zoom || mapLevel->_maxZoom < zoom)
            continue;

        if(bbox31 && !bbox31->intersects(mapLevel->_area31))
            continue;

        // If there are no tree nodes in map level, it means they are not loaded
        {
            QMutexLocker scopedLock(&mapLevel->_d->_rootNodesMutex);

            if(!mapLevel->_d->_rootNodes)
            {
                cis->Seek(mapLevel->_offset);
                auto oldLimit = cis->PushLimit(mapLevel->_length);
                cis->Skip(mapLevel->_boxesInnerOffset);
                mapLevel->_d->_rootNodes.reset(new ObfMapSectionLevel_P::RootNodes());
                readMapLevelTreeNodes(reader, section, mapLevel, mapLevel->_d->_rootNodes->nodes);
                cis->PopLimit(oldLimit);
            }
        }
        
        QList< std::shared_ptr<ObfMapSectionLevelTreeNode> > treeNodesWithData;
        for(auto itTreeNode = mapLevel->_d->_rootNodes->nodes.begin(); itTreeNode != mapLevel->_d->_rootNodes->nodes.end(); ++itTreeNode)
        {
            const auto& treeNode = *itTreeNode;

            if(bbox31 && !bbox31->intersects(treeNode->_area31))
                continue;

            if(treeNode->_dataOffset > 0)
                treeNodesWithData.push_back(treeNode);

            cis->Seek(treeNode->_offset);
            auto oldLimit = cis->PushLimit(treeNode->_length);
            cis->Skip(treeNode->_childrenInnerOffset);
            readTreeNodeChildren(reader, section, treeNode, &treeNodesWithData, bbox31, controller);
            assert(cis->BytesUntilLimit() == 0);
            cis->PopLimit(oldLimit);
        }
        qSort(treeNodesWithData.begin(), treeNodesWithData.end(), [](const std::shared_ptr<ObfMapSectionLevelTreeNode>& l, const std::shared_ptr<ObfMapSectionLevelTreeNode>& r) -> bool
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
