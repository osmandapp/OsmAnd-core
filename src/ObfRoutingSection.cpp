#include "ObfRoutingSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

OsmAnd::ObfRoutingSection::ObfRoutingSection()
    : _borderBoxPointer(0)
    , _baseBorderBoxPointer(0)
    , _borderBoxLength(0)
    , _baseBorderBoxLength(0)
    , _nameTypeRule(-1)
    , _refTypeRule(-1)
{
}

OsmAnd::ObfRoutingSection::~ObfRoutingSection()
{
}

void OsmAnd::ObfRoutingSection::initRouteEncodingRule( int id, std::string tags, std::string val )
{
    while(_routeEncodingRules.size() <= id)
        _routeEncodingRules.push_back(std::shared_ptr<TypeRule>());
    _routeEncodingRules.push_back(std::shared_ptr<TypeRule>(new TypeRule(tags, val)));
    if(tags == "name")
        _nameTypeRule = id;
    else if(tags == "ref")
        _refTypeRule = id;
}

void OsmAnd::ObfRoutingSection::read( gpb::io::CodedInputStream* cis, ObfRoutingSection* section )
{
    int routeEncodingRule = 1;
    for(;;)
    {
        auto tag = cis->ReadTag();
        auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch(tfn)
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex::kNameFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_name);
            break;
        case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readRouteEncodingRule(cis, section, routeEncodingRule++);
                cis->Skip(cis->BytesUntilLimit());
                cis->PopLimit(oldLimit);
            } 
            break;
        case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
        case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                std::shared_ptr<Subregion> subregion(new Subregion(section));
                subregion->_length = ObfReader::readBigEndianInt(cis);
                subregion->_offset = cis->TotalBytesRead();
                auto oldLimit = cis->PushLimit(subregion->_length);
                Subregion::read(cis, subregion.get(), nullptr, 0, true);
                if(tfn == OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber)
                    section->_subregions.push_back(subregion);
                else
                    section->_baseSubregions.push_back(subregion);
                cis->Skip(cis->BytesUntilLimit());
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex::kBaseBorderBoxFieldNumber:
        case OBF::OsmAndRoutingIndex::kBorderBoxFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->TotalBytesRead();
                if(tfn == OBF::OsmAndRoutingIndex::kBorderBoxFieldNumber)
                {
                    section->_borderBoxLength = length;
                    section->_borderBoxPointer = offset;
                }
                else
                {
                    section->_baseBorderBoxLength = length;
                    section->_baseBorderBoxPointer = offset;
                }
                cis->Skip(length);
            }
            break;
        case OBF::OsmAndRoutingIndex::kBlocksFieldNumber:
            // Finish reading file!
            cis->Skip(cis->BytesUntilLimit());
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readRouteEncodingRule( gpb::io::CodedInputStream* cis, ObfRoutingSection* section, int id )
{
    std::string tags;
    std::string val;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            section->initRouteEncodingRule(id, tags, val);
            return;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &val);
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &tags);
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&id));
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

OsmAnd::ObfRoutingSection::Subregion::Subregion( ObfRoutingSection* section )
    : _section(section)
{
}

OsmAnd::ObfRoutingSection::Subregion* OsmAnd::ObfRoutingSection::Subregion::read( gpb::io::CodedInputStream* cis, Subregion* current, Subregion* parent, int depth, bool readCoordinates )
{
    bool readChildren = depth != 0; 

    current->_section->_regionsRead++;
    gpb::int32 value;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return current;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber:
            value = ObfReader::readSInt32(cis);
            if (readCoordinates)
                current->_left = value + (parent ? parent->_left : 0);
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
            value = ObfReader::readSInt32(cis);
            if (readCoordinates)
                current->_right = value + (parent ? parent->_right : 0);
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
            value = ObfReader::readSInt32(cis);
            if (readCoordinates)
                current->_top = value + (parent ? parent->_top : 0);
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
            value = ObfReader::readSInt32(cis);
            if (readCoordinates)
                current->_bottom = value + (parent ? parent->_bottom : 0);
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            {
                current->_shiftToData = ObfReader::readBigEndianInt(cis);
                if(!readChildren)
                    readChildren = true;
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            if(readChildren)
            {
                std::shared_ptr< Subregion > subregion(new Subregion(current->_section));
                subregion->_length = ObfReader::readBigEndianInt(cis);
                subregion->_offset = cis->TotalBytesRead();
                auto oldLimit = cis->PushLimit(subregion->_length);
                Subregion::read(cis, subregion.get(), current, depth - 1, true);
                current->_subregions.push_back(subregion);
                cis->PopLimit(oldLimit);
                cis->Seek(subregion->_offset + subregion->_length);
            }
            else
            {
                cis->Seek(current->_offset + current->_length);
                // skipUnknownField(t);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

OsmAnd::ObfRoutingSection::TypeRule::TypeRule( std::string tag, std::string value )
    : _tag(tag)
    , _value(value)
{
    if(stricmp(_tag.c_str(), "oneway") == 0)
    {
        _type = ONEWAY;
        if( _value == "-1" || _value == "reverse") {
            _intValue = -1;
        } else if(_value == "1" || _value == "yes") {
            _intValue = 1;
        } else {
            _intValue = 0;
        }
    } else if(stricmp(_tag.c_str(), "highway") == 0 && _value == "traffic_signals") {
        _type = TRAFFIC_SIGNALS;
    } else if(stricmp(_tag.c_str(), "railway") == 0 && (_value == "crossing" || _value == "level_crossing")) {
        _type = RAILWAY_CROSSING;
    } else if(stricmp(_tag.c_str(), "roundabout") == 0 && !_value.empty()){
        _type = ROUNDABOUT;
    } else if(stricmp(_tag.c_str(), "junction") == 0 && stricmp(_value.c_str(), "roundabout") == 0){
        _type = ROUNDABOUT;
    } else if(stricmp(_tag.c_str(), "highway") == 0 && !_value.empty()){
        _type = HIGHWAY_TYPE;
    } else if(_tag.find("access") == 0 && !_value.empty()){
        _type = ACCESS;
    } else if(stricmp(_tag.c_str(), "maxspeed") == 0 && !_value.empty()){
        _type = MAXSPEED;
        _floatValue = -1;
        if(_value == "none") {
            //TODO:
            //_floatValue = RouteDataObject.NONE_MAX_SPEED;
        } else {
            int i = 0;
            while (i < _value.length() && isdigit(_value[i])) {
                i++;
            }
            if (i > 0) {
                _floatValue = atoi(_value.substr(0, i).c_str());
                _floatValue /= 3.6f; // km/h -> m/s
                if (_value.find("mph") != std::string::npos) {
                    _floatValue *= 1.6f;
                }
            }
        }
    } else if (stricmp(_tag.c_str(), "lanes") == 0 && !_value.empty()) {
        _intValue = -1;
        int i = 0;
        _type = LANES;
        while (i < _value.length() && isdigit(_value[i]))
            i++;
        if (i > 0)
            _intValue = atoi(_value.substr(0, i).c_str());
    }
}
