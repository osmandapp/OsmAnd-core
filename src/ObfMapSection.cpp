#include "ObfMapSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

namespace gpb = google::protobuf;

OsmAnd::ObfMapSection::ObfMapSection( class ObfReader* owner )
    : ObfSection(owner)
    /*    , _nameEncodingType(0)
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
/*
bool OsmAnd::ObfMapSection::getRule( std::string t, std::string v, int& outRule )
{
auto m = _encodingRules.find(t);
if(m == _encodingRules.end())
return false;
outRule = m->second[v];
return true;
}

std::tuple<std::string, std::string, int> OsmAnd::ObfMapSection::decodeType( int type )
{
return _decodingRules[type];
}

void OsmAnd::ObfMapSection::initMapEncodingRule( int type, int id, std::string tag, std::string val )
{

}
*/
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
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                section->_name = QString::fromStdString(name);
            }
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
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_bottom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_left));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_right));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_top));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_maxZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&level->_minZoom));
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

void OsmAnd::ObfMapSection::queryMapObjects( ObfReader* reader, ObfMapSection* section, QList< std::shared_ptr<MapObject> >* resultOut /*= nullptr*/ )
{
    auto cis = reader->_codedInputStream.get();
    //List<MapTree> foundSubtrees = new ArrayList<MapTree>();

    //TODO: may be moved out
    // Load encoding rules if not yet done so
    if(section->_encodingRules.isEmpty())
    {
        cis->Seek(section->_offset);
        auto oldLimit = cis->PushLimit(section->_length);
        readEncodingRules(reader,
            section->_encodingRules,
            section->_decodingRules,
            section->_nameEncodingType,
            section->_coastlineEncodingType,
            section->_landEncodingType,
            section->_onewayAttribute,
            section->_onewayReverseAttribute,
            section->_refEncodingType,
            section->_coastlineBrokenEncodingType,
            section->_negativeLayers,
            section->_positiveLayers);
        cis->PopLimit(oldLimit);
    }

    for(auto itLevel = section->_levels.begin(); itLevel != section->_levels.end(); ++itLevel)
    {
        auto level = *itLevel;
        /*
        if (!(index.minZoom <= req.zoom && index.maxZoom >= req.zoom))
        continue;

        if (index.right < req.left || index.left > req.right || index.top > req.bottom || index.bottom < req.top) {
        continue;
        }*/
        /*

        // lazy initializing trees
        if(index.trees == null){
        index.trees = new ArrayList<MapTree>();
        codedIS.seek(index.filePointer);
        int oldLimit = codedIS.pushLimit(index.length);
        readMapLevel(index);
        codedIS.popLimit(oldLimit);
        }

        for (MapTree tree : index.trees) {
        if (tree.right < req.left || tree.left > req.right || tree.top > req.bottom || tree.bottom < req.top) {
        continue;
        }
        codedIS.seek(tree.filePointer);
        int oldLimit = codedIS.pushLimit(tree.length);
        searchMapTreeBounds(tree, index, req, foundSubtrees);
        codedIS.popLimit(oldLimit);
        }

        Collections.sort(foundSubtrees, new Comparator<MapTree>() {
        @Override
        public int compare(MapTree o1, MapTree o2) {
        return o1.mapDataBlock < o2.mapDataBlock ? -1 : (o1.mapDataBlock == o2.mapDataBlock ? 0 : 1);
        }
        });
        for(MapTree tree : foundSubtrees) {
        if(!req.isCancelled()){
        codedIS.seek(tree.mapDataBlock);
        int length = codedIS.readRawVarint32();
        int oldLimit = codedIS.pushLimit(length);
        readMapDataBlocks(req, tree, mapIndex);
        codedIS.popLimit(oldLimit);
        }
        }
        foundSubtrees.clear();*/
    }

    //log.info("Search is done. Visit " + req.numberOfVisitedObjects + " objects. Read " + req.numberOfAcceptedObjects + " objects."); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
    //log.info("Read " + req.numberOfReadSubtrees + " subtrees. Go through " + req.numberOfAcceptedSubtrees + " subtrees.");   //$NON-NLS-1$//$NON-NLS-2$//$NON-NLS-3$
    //return req.getSearchResults();
}

void OsmAnd::ObfMapSection::readEncodingRules(
    ObfReader* reader,
    QMap< QString, QMap<QString, uint32_t> >& encodingRules,
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
    QMap< QString, QMap<QString, uint32_t> >& encodingRules,
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
                    encodingRules[ruleTag] = QMap<QString, uint32_t>();
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
            {
                std::string value;
                gpb::internal::WireFormatLite::ReadString(cis, &value);
                ruleVal = QString::fromStdString(value);
            }
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
            {
                std::string value;
                gpb::internal::WireFormatLite::ReadString(cis, &value);
                ruleTag = QString::fromStdString(value);
            }
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

//------------------------------------------------------------------------------------------------------------
/*


void OsmAnd::ObfMapSection::readMapTreeBounds( ObfReader* reader, MapTree* tree, int left, int right, int top, int bottom )
{
auto cis = reader->_codedInputStream.get();

for(;;)
{
auto tag = cis->ReadTag();
switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
{
case 0:
return;
case OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
tree->_bottom = ObfReader::readSInt32(cis) + bottom;
break;
case OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
tree->_top = ObfReader::readSInt32(cis) + top;
break;
case OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
tree->_left = ObfReader::readSInt32(cis) + left;
break;
case OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
tree->_right = ObfReader::readSInt32(cis) + right;
break;   
case OBF::OsmAndMapIndex_MapDataBox::kOceanFieldNumber:
{
gpb::uint32 value;
cis->ReadVarint32(&value);
tree->_isOcean = (value != 0);
}
break;
case OBF::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
tree->_mapDataBlock = ObfReader::readBigEndianInt(cis) + tree->_offset;
break;

default:
ObfReader::skipUnknownField(cis, tag);
break;
}
}
}
*/