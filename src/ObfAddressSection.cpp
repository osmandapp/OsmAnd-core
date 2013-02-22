#include "ObfAddressSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>
#include <Settlement.h>
#include <PostcodeArea.h>
#include <Utilities.h>

#include "OBF.pb.h"

OsmAnd::ObfAddressSection::ObfAddressSection( class ObfReader* owner )
    : ObfSection(owner)
    , _indexNameOffset(-1)
{
}

OsmAnd::ObfAddressSection::~ObfAddressSection()
{
}

void OsmAnd::ObfAddressSection::read( ObfReader* reader, ObfAddressSection* section )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(section->_latinName.isEmpty())
                section->_latinName = reader->transliterate(section->_name);
            return;
        case OBF::OsmAndAddressIndex::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                section->_name = QString::fromStdString(name);
            }
            break;
        case OBF::OsmAndAddressIndex::kNameEnFieldNumber:
            {
                std::string latinName;
                gpb::internal::WireFormatLite::ReadString(cis, &latinName);
                section->_latinName = QString::fromStdString(latinName);
            }
            break;
        case OBF::OsmAndAddressIndex::kCitiesFieldNumber:
            {
                std::shared_ptr<AddressBlocksSection> entry(new AddressBlocksSection(section->_owner));
                entry->_length = ObfReader::readBigEndianInt(cis);
                entry->_offset = cis->CurrentPosition();
                AddressBlocksSection::read(reader, entry.get());
                cis->Seek(entry->_offset + entry->_length);
                section->_blocksSections.push_back(entry);
            }
            break;
        case OBF::OsmAndAddressIndex::kNameIndexFieldNumber:
            {
                section->_indexNameOffset = cis->CurrentPosition();
                auto length = ObfReader::readBigEndianInt(cis);
                cis->Seek(section->_indexNameOffset + length + 4);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }

}

void OsmAnd::ObfAddressSection::readStreetGroups( ObfReader* reader, ObfAddressSection* section, std::list< std::shared_ptr<Model::StreetGroup> >& list, uint8_t typeBitmask /*= std::numeric_limits<uint8_t>::max()*/ )
{
    auto cis = reader->_codedInputStream.get();

    for(auto itAddressBlocksSection = section->_blocksSections.begin(); itAddressBlocksSection != section->_blocksSections.end(); ++itAddressBlocksSection)
    {
        auto block = *itAddressBlocksSection;
        if((typeBitmask & (1 << block->_type)) == 0)
            continue;

        auto res = cis->Seek(block->_offset);
        auto oldLimit = cis->PushLimit(block->_length);
        AddressBlocksSection::readStreetGroups(reader, block.get(), list);
        cis->PopLimit(oldLimit);
    }
}

void OsmAnd::ObfAddressSection::loadStreetGroups( std::list< std::shared_ptr<Model::StreetGroup> >& list, uint8_t typeBitmask /*= std::numeric_limits<uint8_t>::max()*/ )
{
    readStreetGroups(_owner, this, list, typeBitmask);
}

void OsmAnd::ObfAddressSection::loadStreetsFromGroup( ObfReader* reader, Model::StreetGroup* group, std::list< std::shared_ptr<Model::Street> >& list )
{
    //checkAddressIndex(c.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(group->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readStreetsFromGroup(reader, group, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readStreetsFromGroup( ObfReader* reader, Model::StreetGroup* group, std::list< std::shared_ptr<Model::Street> >& list )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::CityBlockIndex::kStreetsFieldNumber:
            {
                std::shared_ptr<Model::Street> street(new Model::Street(/*group*/));
                street->_offset = cis->CurrentPosition();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readStreet(reader, group, street.get());
                if(/*resultMatcher == null || resultMatcher.publish(s)*/true)
                {
                    //TODO:group->registerStreet(s);
                    list.push_back(street);
                }
                /*if(resultMatcher != null && resultMatcher.isCancelled()) {
                    codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                }*/
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::CityBlockIndex::kBuildingsFieldNumber:
            // buildings for the town are not used now
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::readStreet( ObfReader* reader, Model::StreetGroup* group, Model::Street* street)
{
    auto cis = reader->_codedInputStream.get();

    const auto x24 = Utilities::get31TileNumberX(group->_longitude) >> 7;
    const auto y24 = Utilities::get31TileNumberY(group->_latitude) >> 7;
    //boolean loadLocation = city24X != 0 || city24Y != 0;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(street->_latinName.isEmpty())
                street->_latinName = reader->transliterate(street->_name);
            return;
        case OBF::StreetIndex::kIdFieldNumber:
            {
                gpb::uint64 id;
                cis->ReadVarint64(&id);
                street->_id = id;
            }
            break;
        case OBF::StreetIndex::kNameEnFieldNumber:
            {
                std::string latinName;
                gpb::internal::WireFormatLite::ReadString(cis, &latinName);
                street->_latinName = QString::fromStdString(latinName);
            }
            break;
        case OBF::StreetIndex::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                street->_name = QString::fromStdString(name);
            }
            break;
        /*TODO:case OBF::StreetIndex::X_FIELD_NUMBER :
            {

            int sx = codedIS.readSInt32();
            if(loadLocation){
                x =  sx + city24X;
            } else {
                x = (int) MapUtils.getTileNumberX(24, s.getLocation().getLongitude());
            }
            }
            break;
        case OBF::StreetIndex::Y_FIELD_NUMBER :
            int sy = codedIS.readSInt32();
            if(loadLocation){
                y =  sy + city24Y;
            } else {
                y = (int) MapUtils.getTileNumberY(24, s.getLocation().getLatitude());
            }
            break;
        case OBF::StreetIndex::INTERSECTIONS_FIELD_NUMBER :
            int length = codedIS.readRawVarint32();
            if(loadBuildingsAndIntersected){
                int oldLimit = codedIS.pushLimit(length);
                Street si = readIntersectedStreet(s.getCity(), x, y);
                s.addIntersectedStreet(si);
                codedIS.popLimit(oldLimit);
            } else {
                codedIS.skipRawBytes(length);
            }
            break;
        case OBF::StreetIndex::BUILDINGS_FIELD_NUMBER :
            int offset = codedIS.getTotalBytesRead();
            length = codedIS.readRawVarint32();
            if(loadBuildingsAndIntersected){
                int oldLimit = codedIS.pushLimit(length);
                Building b = readBuilding(offset, x, y);
                if (postcodeFilter == null || postcodeFilter.equalsIgnoreCase(b.getPostcode())) {
                    if (buildingsMatcher == null || buildingsMatcher.publish(b)) {
                        s.registerBuilding(b);
                    }
                }
                codedIS.popLimit(oldLimit);
            } else {
                codedIS.skipRawBytes(length);
            }
            break;*/
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::loadBuildingsFromStreet( ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Building> >& list )
{
    //checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    //addressAdapter.readStreet(s, resultMatcher, true, 0, 0, city != null  && city.isPostcode() ? city.getName() : null);
    readBuildingsFromStreet(reader, street, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readBuildingsFromStreet( ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Building> >& list )
{

}

OsmAnd::ObfAddressSection::AddressBlocksSection::AddressBlocksSection( class ObfReader* owner )
    : ObfSection(owner)
{
}

OsmAnd::ObfAddressSection::AddressBlocksSection::~AddressBlocksSection()
{
}

void OsmAnd::ObfAddressSection::AddressBlocksSection::read( ObfReader* reader, AddressBlocksSection* section )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_type));
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::AddressBlocksSection::readStreetGroups( ObfReader* reader, OsmAnd::ObfAddressSection::AddressBlocksSection* section, std::list< std::shared_ptr<Model::StreetGroup> >& list )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndAddressIndex_CitiesIndex::kCitiesFieldNumber:
            {
                auto offset = cis->CurrentPosition();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto streetGroup = readStreetGroupHeader(reader, section, offset);
                if(streetGroup)
                {
                    if(/*resultMatcher == null || resultMatcher.publish(c)*/true)
                        list.push_back(streetGroup);
                }
                cis->PopLimit(oldLimit);
                /*(if(resultMatcher != null && resultMatcher.isCancelled()){
                    codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                }*/
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::AddressBlocksSection::loadSteetGroupsFromBlock( std::list< std::shared_ptr<Model::StreetGroup> >& list )
{
    auto cis = _owner->_codedInputStream.get();

    cis->Seek(_offset);
    auto oldLimit = cis->PushLimit(_length);
    AddressBlocksSection::readStreetGroups(_owner, this, list);
    cis->PopLimit(oldLimit);
}

std::shared_ptr<OsmAnd::Model::StreetGroup> OsmAnd::ObfAddressSection::AddressBlocksSection::readStreetGroupHeader( ObfReader* reader, OsmAnd::ObfAddressSection::AddressBlocksSection* section, unsigned int offset )
{
    auto cis = reader->_codedInputStream.get();

    std::shared_ptr<OsmAnd::Model::StreetGroup> streetGroup;
//    boolean englishNameMatched = false;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(streetGroup->_latinName.isEmpty())
                streetGroup->_latinName = section->_owner->transliterate(streetGroup->_name);
            return streetGroup;
        case OBF::CityIndex::kCityTypeFieldNumber:
            {
                gpb::uint32 type;
                cis->ReadVarint32(&type);
                streetGroup.reset(new Model::Settlement(/*TODO:type*/));
            }
            break;
        case OBF::CityIndex::kIdFieldNumber:
            {
                gpb::uint64 id;
                cis->ReadVarint64(&id);
                streetGroup->_id = id;
            }
            /*
            TODO:
            if(nameMatcher != null && useEn && !englishNameMatched){
                codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                return null;
            }
            */
            break;
        case OBF::CityIndex::kNameEnFieldNumber:
            {
                std::string latinName;
                gpb::internal::WireFormatLite::ReadString(cis, &latinName);
                /*
                TODO:
                if (nameMatcher != null && latinName.length() > 0 && nameMatcher.matches(latinName)) {
                englishNameMatched = true;
                }
                */
                streetGroup->_latinName = QString::fromStdString(latinName);
            }
            break;
        case OBF::CityIndex::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                /*
                if(nameMatcher != null){
                if(!useEn){
                if(!nameMatcher.matches(name)) {
                codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                return null;
                }
                } else if(nameMatcher.matches(Junidecode.unidecode(name))){
                englishNameMatched = true;
                }
                }

                if(c == null) {
                c = City.createPostcode(name); 
                }
                */

                if(!streetGroup)
                    streetGroup.reset(new Model::PostcodeArea());

                streetGroup->_name = QString::fromStdString(name);
            }
            break;
        case OBF::CityIndex::kXFieldNumber:
            {
                gpb::uint32 x = 0;
                cis->ReadVarint32(&x);
                streetGroup->_longitude = Utilities::get31LongitudeX(x);
            }
            break;
        case OBF::CityIndex::kYFieldNumber:
            {
                gpb::uint32 y = 0;
                cis->ReadVarint32(&y);
                streetGroup->_latitude = Utilities::get31LatitudeY(y);
            }
            break;
        case OBF::CityIndex::kShiftToCityBlockIndexFieldNumber:
            streetGroup->_offset = ObfReader::readBigEndianInt(cis) + offset;
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}
