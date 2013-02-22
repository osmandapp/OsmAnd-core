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
        case OBF::StreetIndex::kXFieldNumber:
            {
                auto dx = ObfReader::readSInt32(cis);
                auto x = (Utilities::get31TileNumberX(group->_longitude) >> 7) + dx;
                street->_longitude = Utilities::getLongitudeFromTile(24, x);
            }
            break;
        case OBF::StreetIndex::kYFieldNumber:
            {
                auto dy = ObfReader::readSInt32(cis);
                auto y = (Utilities::get31TileNumberY(group->_latitude) >> 7) + dy;
                street->_latitude = Utilities::getLatitudeFromTile(24, y);
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

void OsmAnd::ObfAddressSection::loadBuildingsFromStreet( ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Building> >& list )
{
    //checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readBuildingsFromStreet(reader, street, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readBuildingsFromStreet( ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Building> >& list )
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
                readBuilding(reader, building.get());
                if (/*postcodeFilter == null || postcodeFilter.equalsIgnoreCase(b.getPostcode())*/true)
                {
                    if (/*buildingsMatcher == null || buildingsMatcher.publish(b)*/true)
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

void OsmAnd::ObfAddressSection::readBuilding( ObfReader* reader, Model::Building* building )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            //b.setLocation(MapUtils.getLatitudeFromTile(24, y), MapUtils.getLongitudeFromTile(24, x));
            if(building->_latinName.isEmpty())
                building->_latinName = reader->transliterate(building->_name);
            if(building->_latinName2.isEmpty())
                building->_latinName2 = reader->transliterate(building->_name2);
            return;
        case OBF::BuildingIndex::kIdFieldNumber:
            cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&building->_id));
            break;
        case OBF::BuildingIndex::kNameEnFieldNumber:
            {
                std::string latinName;
                gpb::internal::WireFormatLite::ReadString(cis, &latinName);
                building->_latinName = QString::fromStdString(latinName);
            }
            break;
        case OBF::BuildingIndex::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                building->_name = QString::fromStdString(name);
            }
            break;
        case OBF::BuildingIndex::kNameEn2FieldNumber:
            {
                std::string latinName;
                gpb::internal::WireFormatLite::ReadString(cis, &latinName);
                building->_latinName2 = QString::fromStdString(latinName);
            }
            break;
        case OBF::BuildingIndex::kName2FieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                building->_name2 = QString::fromStdString(name);
            }
            break;
        /*case OsmandOdb.BuildingIndex.INTERPOLATION_FIELD_NUMBER :
            int sint = codedIS.readSInt32();
            if(sint > 0) {
                b.setInterpolationInterval(sint);
            } else {
                b.setInterpolationType(BuildingInterpolation.fromValue(sint));
            }
            break;
        case OsmandOdb.BuildingIndex.X_FIELD_NUMBER :
            x =  codedIS.readSInt32() + street24X;
            break;
        case OsmandOdb.BuildingIndex.Y_FIELD_NUMBER :
            y =  codedIS.readSInt32() + street24Y;
            break;
        case OsmandOdb.BuildingIndex.POSTCODE_FIELD_NUMBER :
            b.setPostcode(codedIS.readString());
            break;*/
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSection::loadIntersectionsFromStreet( ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Street::IntersectedStreet> >& list )
{
    //checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readIntersectionsFromStreet(reader, street, list);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSection::readIntersectionsFromStreet( ObfReader* reader, Model::Street* street, std::list< std::shared_ptr<Model::Street::IntersectedStreet> >& list )
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
                std::shared_ptr<Model::Street::IntersectedStreet> intersectedStreet(new Model::Street::IntersectedStreet());
                readIntersectedStreet(reader, intersectedStreet.get());
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

void OsmAnd::ObfAddressSection::readIntersectedStreet( ObfReader* reader, Model::Street::IntersectedStreet* street )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            //s.setLocation(MapUtils.getLatitudeFromTile(24, y), MapUtils.getLongitudeFromTile(24, x));
            if(street->_latinName.isEmpty())
                street->_latinName = reader->transliterate(street->_name);
            return;
        case OBF::StreetIntersection::kNameEnFieldNumber:
            {
                std::string latinName;
                gpb::internal::WireFormatLite::ReadString(cis, &latinName);
                street->_latinName = QString::fromStdString(latinName);
            }
            break;
        case OBF::StreetIntersection::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                street->_name = QString::fromStdString(name);
            }
            break;
        /*case OBF::StreetIntersection.INTERSECTEDX_FIELD_NUMBER :
            x =  codedIS.readSInt32() + street24X;
            break;
        case OsmandOdb.StreetIntersection.INTERSECTEDY_FIELD_NUMBER :
            y =  codedIS.readSInt32() + street24Y;
            break;*/
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
