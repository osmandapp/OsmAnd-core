#include "ObfMapSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

namespace gpb = google::protobuf;

OsmAnd::ObfMapSection::ObfMapSection()
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

OsmAnd::ObfMapSection::~ObfMapSection()
{
}

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

bool OsmAnd::ObfMapSection::isBaseMap()
{
#if defined(_MSC_VER)
    return stricmp(_name.c_str(), "basemap") == 0;
#else
    return strncasecmp(_name.c_str(), "basemap") == 0;
#endif
}

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

void OsmAnd::ObfMapSection::read( gpb::io::CodedInputStream* cis, ObfMapSection* section, bool onlyInitEncodingRules )
{
    int defaultId = 1;
    for(;;)
    {
        gpb::uint32 tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if (onlyInitEncodingRules)
                section->finishInitializingTags();
            return;
        case OBF::OsmAndMapIndex::kNameFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_name);
            break;
        case OBF::OsmAndMapIndex::kRulesFieldNumber:
            if (onlyInitEncodingRules)
            {
                gpb::uint32 len;
                cis->ReadVarint32(&len);
                auto oldLimit = cis->PushLimit(len);
                readMapEncodingRule(cis, section, defaultId++);
                cis->PopLimit(oldLimit);
            }
            else
                ObfReader::skipUnknownField(cis, tag);
            break;
        case OBF::OsmAndMapIndex::kLevelsFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->TotalBytesRead();
                if (!onlyInitEncodingRules)
                {
                    auto oldLimit = cis->PushLimit(length);
                    std::shared_ptr<MapRoot> mapRoot(readMapLevel(cis, new MapRoot(), false));
                    mapRoot->_length = length;
                    mapRoot->_offset = offset;
                    section->_levels.push_back(mapRoot);
                    cis->PopLimit(oldLimit);
                }
                cis->Seek(offset + length);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}


void OsmAnd::ObfMapSection::readMapEncodingRule( gpb::io::CodedInputStream* cis, ObfMapSection* section, int id )
{
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

OsmAnd::ObfMapSection::MapRoot* OsmAnd::ObfMapSection::readMapLevel( gpb::io::CodedInputStream* cis, MapRoot* root, bool initSubtrees )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return root;
        case OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&root->_bottom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&root->_left));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&root->_right));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&root->_top));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&root->_maxZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&root->_minZoom));
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->TotalBytesRead();
                if (initSubtrees)
                {
                    std::shared_ptr<MapTree> r(new MapTree());
                    r->_length = length;
                    r->_offset= offset;
                    auto oldLimit = cis->PushLimit(length);
                    readMapTreeBounds(cis, r.get(), root->_left, root->_right, root->_top, root->_bottom);
                    root->_trees.push_back(r);
                    cis->PopLimit(oldLimit);
                }
                cis->Seek(offset + length);
            }
            break;
        case OBF::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfMapSection::readMapTreeBounds( gpb::io::CodedInputStream* cis, MapTree* tree, int left, int right, int top, int bottom )
{
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

OsmAnd::ObfMapSection::MapTree::MapTree()
    : _offset(0)
    , _length(0)
    , _mapDataBlock(0)
    , _isOcean(false)
    , _left(0)
    , _right(0)
    , _top(0)
    , _bottom(0)
{
}
