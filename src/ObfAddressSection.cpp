#include "ObfAddressSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>
#include <Settlement.h>
#include <Street.h>
#include <StreetGroup.h>
#include <StreetIntersection.h>
#include <Building.h>
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
            ObfReader::readQString(cis, section->_name);
            break;
        case OBF::OsmAndAddressIndex::kNameEnFieldNumber:
            ObfReader::readQString(cis, section->_latinName);
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

void OsmAnd::ObfAddressSection::readStreetGroups( ObfReader* reader, ObfAddressSection* section, QList< std::shared_ptr<Model::StreetGroup> >& list, uint8_t typeBitmask /*= std::numeric_limits<uint8_t>::max()*/ )
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

void OsmAnd::ObfAddressSection::loadStreetGroups( ObfReader* reader, ObfAddressSection* section, QList< std::shared_ptr<Model::StreetGroup> >& list, uint8_t typeBitmask /*= std::numeric_limits<uint8_t>::max()*/ )
{
    readStreetGroups(reader, section, list, typeBitmask);
}

void OsmAnd::ObfAddressSection::loadStreetsFromGroup( ObfReader* reader, Model::StreetGroup* group, QList< std::shared_ptr<Model::Street> >& list )
{
    //TODO:checkAddressIndex(c.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(group->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readStreetsFromGroup(reader, group, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readStreetsFromGroup( ObfReader* reader, Model::StreetGroup* group, QList< std::shared_ptr<Model::Street> >& list )
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
                std::shared_ptr<Model::Street> street(new Model::Street(/*TODO:group*/));
                street->_offset = cis->CurrentPosition();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readStreet(reader, group, street.get());
                if(/*TODO:resultMatcher == null || resultMatcher.publish(s)*/true)
                {
                    //TODO:group->registerStreet(s);
                    list.push_back(street);
                }
                /*TODO:if(resultMatcher != null && resultMatcher.isCancelled()) {
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
            cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&street->_id));
            break;
        case OBF::StreetIndex::kNameEnFieldNumber:
            ObfReader::readQString(cis, street->_latinName);
            break;
        case OBF::StreetIndex::kNameFieldNumber:
            ObfReader::readQString(cis, street->_name);
            break;
        case OBF::StreetIndex::kXFieldNumber:
            {
                auto dx = ObfReader::readSInt32(cis);
                street->_tile24.x = (Utilities::get31TileNumberX(group->_longitude) >> 7) + dx;
                street->_location.x = Utilities::getLongitudeFromTile(24, street->_tile24.x);
            }
            break;
        case OBF::StreetIndex::kYFieldNumber:
            {
                auto dy = ObfReader::readSInt32(cis);
                street->_tile24.y = (Utilities::get31TileNumberY(group->_latitude) >> 7) + dy;
                street->_location.y = Utilities::getLatitudeFromTile(24, street->_tile24.y);
            }
            break;
        case OBF::StreetIndex::kIntersectionsFieldNumber:
        case OBF::StreetIndex::kBuildingsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                cis->Skip(length);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::loadBuildingsFromStreet( ObfReader* reader, Model::Street* street, QList< std::shared_ptr<Model::Building> >& list )
{
    //TODO:checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readBuildingsFromStreet(reader, street, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readBuildingsFromStreet( ObfReader* reader, Model::Street* street, QList< std::shared_ptr<Model::Building> >& list )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::StreetIndex::kIntersectionsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                cis->Skip(length);
            }
            break;
        case OBF::StreetIndex::kBuildingsFieldNumber:
            {
                auto offset = cis->CurrentPosition();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<Model::Building> building(new Model::Building());
                building->_offset = offset;
                readBuilding(reader, street, building.get());
                if (/*TODO:postcodeFilter == null || postcodeFilter.equalsIgnoreCase(b.getPostcode())*/true)
                {
                    if (/*TODO:buildingsMatcher == null || buildingsMatcher.publish(b)*/true)
                    {
                        //s.registerBuilding(b);
                        list.push_back(building);
                    }
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

void OsmAnd::ObfAddressSection::readBuilding( ObfReader* reader, Model::Street* street, Model::Building* building )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(building->_latinName.isEmpty())
                building->_latinName = reader->transliterate(building->_name);
            if(building->_latinName2.isEmpty())
                building->_latinName2 = reader->transliterate(building->_name2);
            return;
        case OBF::BuildingIndex::kIdFieldNumber:
            cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&building->_id));
            break;
        case OBF::BuildingIndex::kNameEnFieldNumber:
            ObfReader::readQString(cis, building->_latinName);
            break;
        case OBF::BuildingIndex::kNameFieldNumber:
            ObfReader::readQString(cis, building->_name);
            break;
        case OBF::BuildingIndex::kNameEn2FieldNumber:
            ObfReader::readQString(cis, building->_latinName2);
            break;
        case OBF::BuildingIndex::kName2FieldNumber:
            ObfReader::readQString(cis, building->_name2);
            break;
        case OBF::BuildingIndex::kInterpolationFieldNumber:
            {
                auto value = ObfReader::readSInt32(cis);
                if(value > 0)
                    building->_interpolationInterval = value;
                else
                    building->_interpolation = static_cast<Model::Building::Interpolation>(value);
            }
            break;
        case OBF::BuildingIndex::kXFieldNumber:
            {
                auto dx = ObfReader::readSInt32(cis);
                building->_xTile24 = street->_tile24.x + dx;
            }
            break;
        case OBF::BuildingIndex::kX2FieldNumber:
            {
                auto dx2 = ObfReader::readSInt32(cis);
                building->_x2Tile24 = street->_tile24.x + dx2;
            }
            break;
        case OBF::BuildingIndex::kYFieldNumber:
            {
                auto dy = ObfReader::readSInt32(cis);
                building->_yTile24 = street->_tile24.y + dy;
            }
            break;
        case OBF::BuildingIndex::kY2FieldNumber:
            {
                auto dy2 = ObfReader::readSInt32(cis);
                building->_y2Tile24 = street->_tile24.y + dy2;
            }
            break;
        case OBF::BuildingIndex::kPostcodeFieldNumber:
            ObfReader::readQString(cis, building->_postcode);
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::loadIntersectionsFromStreet( ObfReader* reader, Model::Street* street, QList< std::shared_ptr<Model::StreetIntersection> >& list )
{
    //TODO:checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readIntersectionsFromStreet(reader, street, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readIntersectionsFromStreet( ObfReader* reader, Model::Street* street, QList< std::shared_ptr<Model::StreetIntersection> >& list )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::StreetIndex::kIntersectionsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<Model::StreetIntersection> intersectedStreet(new Model::StreetIntersection());
                readIntersectedStreet(reader, street, intersectedStreet.get());
                list.push_back(intersectedStreet);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::StreetIndex::kBuildingsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                cis->Skip(length);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::readIntersectedStreet( ObfReader* reader, Model::Street* street, Model::StreetIntersection* intersection )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(intersection->_latinName.isEmpty())
                intersection->_latinName = reader->transliterate(intersection->_name);
            return;
        case OBF::StreetIntersection::kNameEnFieldNumber:
            ObfReader::readQString(cis, intersection->_latinName);
            break;
        case OBF::StreetIntersection::kNameFieldNumber:
            ObfReader::readQString(cis, intersection->_name);
            break;
        case OBF::StreetIntersection::kIntersectedXFieldNumber:
            {
                auto dx = ObfReader::readSInt32(cis);
                intersection->_tile24.x = street->_tile24.x + dx;
                intersection->_location.x = Utilities::getLongitudeFromTile(24, intersection->_tile24.x);
            }
            break;
        case OBF::StreetIntersection::kIntersectedYFieldNumber:
            {
                auto dy = ObfReader::readSInt32(cis);
                intersection->_tile24.y = street->_tile24.y + dy;
                street->_location.y = Utilities::getLatitudeFromTile(24, intersection->_tile24.y);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
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

void OsmAnd::ObfAddressSection::AddressBlocksSection::readStreetGroups( ObfReader* reader, OsmAnd::ObfAddressSection::AddressBlocksSection* section, QList< std::shared_ptr<Model::StreetGroup> >& list )
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

void OsmAnd::ObfAddressSection::AddressBlocksSection::loadSteetGroupsFromBlock( QList< std::shared_ptr<Model::StreetGroup> >& list )
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
            cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&streetGroup->_id));
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
                ObfReader::readQString(cis, streetGroup->_latinName);
                /*
                TODO:
                if (nameMatcher != null && latinName.length() > 0 && nameMatcher.matches(latinName)) {
                englishNameMatched = true;
                }
                */
            }
            break;
        case OBF::CityIndex::kNameFieldNumber:
            {
                QString name;
                ObfReader::readQString(cis, name);
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
                */

                if(!streetGroup)
                    streetGroup.reset(new Model::PostcodeArea());

                streetGroup->_name = name;
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
