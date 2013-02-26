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
    if(_encodingRules.find(tag) == _encodingRules.end()) {
        _encodingRules[tag] = std::map<std::string, int>();
    }
    _encodingRules[tag][val] = id;
    if(_decodingRules.find(id) == _decodingRules.end()) {
        _decodingRules[id] = std::tuple<std::string, std::string, int>(tag, val, type);
    }

    if("name" == tag)
        _nameEncodingType = id;
    else if("natural" == tag && "coastline" == val)
        _coastlineEncodingType = id;
    else if("natural" == tag && "land" == val)
        _landEncodingType = id;
    else if("oneway" == tag && "yes" == val)
        _onewayAttribute = id;
    else if("oneway" == tag && "-1" == val)
        _onewayReverseAttribute = id;
    else if("ref" == tag)
        _refEncodingType = id;
    else if("tunnel" == tag)
        _negativeLayers.insert(id);
    else if("bridge" == tag)
        _positiveLayers.insert(id);
    else if("layer" == tag)
    {
        if(!val.empty() && val != "0")
        {
            if (val[0] == '-')
                _negativeLayers.insert(id);
            else
                _positiveLayers.insert(id);
        }
    }
}
*/
bool OsmAnd::ObfMapSection::isBaseMap()
{
    return QString::compare(_name, "basemap", Qt::CaseInsensitive);
}
/*
void OsmAnd::ObfMapSection::finishInitializingTags()
{
    auto free = _decodingRules.size() * 2 + 1;
    _coastlineBrokenEncodingType = free++;
    initMapEncodingRule(0, _coastlineBrokenEncodingType, "natural", "coastline_broken");
    if(_landEncodingType == -1)
    {
        _landEncodingType = free++;
        initMapEncodingRule(0, _landEncodingType, "natural", "land");
    }
}
*/
void OsmAnd::ObfMapSection::read( ObfReader* reader, ObfMapSection* section )
{
    auto cis = reader->_codedInputStream.get();

    int defaultId = 1;
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
//------------------------------------------------------------------------------------------------------------
/*
void OsmAnd::ObfMapSection::readMapEncodingRule( ObfReader* reader, ObfMapSection* section, int id )
{
    auto cis = reader->_codedInputStream.get();

    gpb::uint32 type = 0;
    std::string tags;
    std::string val;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            section->initMapEncodingRule(type, id, tags, val);
            return;
        case OBF::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &val);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &tags);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kTypeFieldNumber:
            cis->ReadVarint32(&type);
            break;
        case OBF::OsmAndMapIndex_MapEncodingRule::kIdFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&id));
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

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