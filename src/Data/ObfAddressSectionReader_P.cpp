#include "ObfAddressSectionReader_P.h"

#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfAddressSectionInfo.h"
#include "Street.h"
#include "StreetGroup.h"
#include "Settlement.h"
#include "PostcodeArea.h"
#include "StreetIntersection.h"
#include "Building.h"
#include "ObfReaderUtilities.h"
#include "Utilities.h"

#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>

OsmAnd::ObfAddressSectionReader_P::ObfAddressSectionReader_P()
{
}

OsmAnd::ObfAddressSectionReader_P::~ObfAddressSectionReader_P()
{
}

void OsmAnd::ObfAddressSectionReader_P::read( const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfAddressSectionInfo>& section )
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
            ObfReaderUtilities::readQString(cis, section->_name);
            break;
        case OBF::OsmAndAddressIndex::kNameEnFieldNumber:
            ObfReaderUtilities::readQString(cis, section->_latinName);
            break;
        case OBF::OsmAndAddressIndex::kCitiesFieldNumber:
            {
                std::shared_ptr<ObfAddressBlocksSectionInfo> addressBlocksSection(new ObfAddressBlocksSectionInfo(section, section->owner));
                addressBlocksSection->_length = ObfReaderUtilities::readBigEndianInt(cis);
                addressBlocksSection->_offset = cis->CurrentPosition();
                readAddressBlocksSectionHeader(reader, addressBlocksSection);
                cis->Seek(addressBlocksSection->_offset + addressBlocksSection->_length);
                section->_addressBlocksSections.push_back(addressBlocksSection);
            }
            break;
        case OBF::OsmAndAddressIndex::kNameIndexFieldNumber:
            {
                auto indexNameOffset = cis->CurrentPosition();
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                cis->Seek(indexNameOffset + length + 4);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readAddressBlocksSectionHeader( const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfAddressBlocksSectionInfo>& section )
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetGroups(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor,
    const IQueryController* const controller,
    QSet<ObfAddressBlockType>* blockTypeFilter /*= nullptr*/ )
{
    auto cis = reader->_codedInputStream.get();

    for(auto itAddressBlocksSection = section->addressBlocksSections.cbegin(); itAddressBlocksSection != section->addressBlocksSections.cend(); ++itAddressBlocksSection)
    {
        if(controller && controller->isAborted())
            break;

        const auto& block = *itAddressBlocksSection;
        if(blockTypeFilter && !blockTypeFilter->contains(block->type))
            continue;

        auto res = cis->Seek(block->_offset);
        auto oldLimit = cis->PushLimit(block->_length);
        readStreetGroupsFromAddressBlocksSection(reader, block, resultOut, visitor, controller);
        cis->PopLimit(oldLimit);
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetGroupsFromAddressBlocksSection(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
    QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor, const IQueryController* const controller )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        if(controller && controller->isAborted())
            break;

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
                std::shared_ptr<OsmAnd::Model::StreetGroup> streetGroup;
                readStreetGroupHeader(reader, section, offset, streetGroup);
                if(streetGroup)
                {
                    if(!visitor || visitor(streetGroup))
                    {
                        if(resultOut)
                            resultOut->push_back(streetGroup);
                    }
                }
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetGroupHeader(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
    unsigned int offset, std::shared_ptr<OsmAnd::Model::StreetGroup>& outStreetGroup )
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
                streetGroup->_latinName = reader->transliterate(streetGroup->_name);
            outStreetGroup = streetGroup;
            return;
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
                ObfReaderUtilities::readQString(cis, streetGroup->_latinName);
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
                ObfReaderUtilities::readQString(cis, name);
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
            streetGroup->_offset = ObfReaderUtilities::readBigEndianInt(cis) + offset;
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::loadStreetGroups(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor,
    const IQueryController* const controller,
    QSet<ObfAddressBlockType>* blockTypeFilter /*= nullptr*/ )
{
    readStreetGroups(reader, section, resultOut, visitor, controller, blockTypeFilter);
}

void OsmAnd::ObfAddressSectionReader_P::loadStreetsFromGroup(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
    QList< std::shared_ptr<const Model::Street> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/)
{
    //TODO:checkAddressIndex(c.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(group->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readStreetsFromGroup(reader, group, resultOut, visitor, controller);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSectionReader_P::readStreetsFromGroup(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
    QList< std::shared_ptr<const Model::Street> >* resultOut,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor,
    const IQueryController* const controller)
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        if(controller && controller->isAborted())
            return;

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
                readStreet(reader, group, street);
                if(!visitor || visitor(street))
                {
                    if(resultOut)
                        resultOut->push_back(street);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::CityBlockIndex::kBuildingsFieldNumber:
            // buildings for the town are not used now
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreet(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
    const std::shared_ptr<Model::Street>& street)
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
            ObfReaderUtilities::readQString(cis, street->_latinName);
            break;
        case OBF::StreetIndex::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, street->_name);
            break;
        case OBF::StreetIndex::kXFieldNumber:
            {
                auto dx = ObfReaderUtilities::readSInt32(cis);
                street->_tile24.x = (Utilities::get31TileNumberX(group->_longitude) >> 7) + dx;
            }
            break;
        case OBF::StreetIndex::kYFieldNumber:
            {
                auto dy = ObfReaderUtilities::readSInt32(cis);
                street->_tile24.y = (Utilities::get31TileNumberY(group->_latitude) >> 7) + dy;
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::loadBuildingsFromStreet(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
    QList< std::shared_ptr<const Model::Building> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/)
{
    //TODO:checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readBuildingsFromStreet(reader, street, resultOut, visitor, controller);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSectionReader_P::readBuildingsFromStreet(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
    QList< std::shared_ptr<const Model::Building> >* resultOut,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor,
    const IQueryController* const controller)
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        if(controller && controller->isAborted())
            return;

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
                readBuilding(reader, street, building);
                if (!visitor || visitor(building))
                {
                    if(resultOut)
                        resultOut->push_back(building);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readBuilding(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
    const std::shared_ptr<Model::Building>& building )
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
            ObfReaderUtilities::readQString(cis, building->_latinName);
            break;
        case OBF::BuildingIndex::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, building->_name);
            break;
        case OBF::BuildingIndex::kNameEn2FieldNumber:
            ObfReaderUtilities::readQString(cis, building->_latinName2);
            break;
        case OBF::BuildingIndex::kName2FieldNumber:
            ObfReaderUtilities::readQString(cis, building->_name2);
            break;
        case OBF::BuildingIndex::kInterpolationFieldNumber:
            {
                auto value = ObfReaderUtilities::readSInt32(cis);
                if(value > 0)
                    building->_interpolationInterval = value;
                else
                    building->_interpolation = static_cast<Model::Building::Interpolation>(value);
            }
            break;
        case OBF::BuildingIndex::kXFieldNumber:
            {
                auto dx = ObfReaderUtilities::readSInt32(cis);
                building->_xTile24 = street->_tile24.x + dx;
            }
            break;
        case OBF::BuildingIndex::kX2FieldNumber:
            {
                auto dx2 = ObfReaderUtilities::readSInt32(cis);
                building->_x2Tile24 = street->_tile24.x + dx2;
            }
            break;
        case OBF::BuildingIndex::kYFieldNumber:
            {
                auto dy = ObfReaderUtilities::readSInt32(cis);
                building->_yTile24 = street->_tile24.y + dy;
            }
            break;
        case OBF::BuildingIndex::kY2FieldNumber:
            {
                auto dy2 = ObfReaderUtilities::readSInt32(cis);
                building->_y2Tile24 = street->_tile24.y + dy2;
            }
            break;
        case OBF::BuildingIndex::kPostcodeFieldNumber:
            ObfReaderUtilities::readQString(cis, building->_postcode);
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::loadIntersectionsFromStreet(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
    QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/)
{
    //TODO:checkAddressIndex(s.getFileOffset());
    auto cis = reader->_codedInputStream;
    cis->Seek(street->_offset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readIntersectionsFromStreet(reader, street, resultOut, visitor, controller);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSectionReader_P::readIntersectionsFromStreet(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
    QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor,
    const IQueryController* const controller)
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        if(controller && controller->isAborted())
            return;

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
                readIntersectedStreet(reader, street, intersectedStreet);
                if(!visitor || visitor(intersectedStreet))
                {
                    if(resultOut)
                        resultOut->push_back(intersectedStreet);
                }
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readIntersectedStreet(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
    const std::shared_ptr<Model::StreetIntersection>& intersection )
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
            ObfReaderUtilities::readQString(cis, intersection->_latinName);
            break;
        case OBF::StreetIntersection::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, intersection->_name);
            break;
        case OBF::StreetIntersection::kIntersectedXFieldNumber:
            {
                auto dx = ObfReaderUtilities::readSInt32(cis);
                intersection->_tile24.x = street->_tile24.x + dx;
            }
            break;
        case OBF::StreetIntersection::kIntersectedYFieldNumber:
            {
                auto dy = ObfReaderUtilities::readSInt32(cis);
                intersection->_tile24.y = street->_tile24.y + dy;
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}
