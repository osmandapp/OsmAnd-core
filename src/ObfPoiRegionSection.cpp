#include "ObfPoiRegionSection.h"

#include <QtEndian>
#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "../native/src/proto/osmand_odb.pb.h"

void OsmAnd::ObfPoiRegionSection::read( gpb::io::CodedInputStream* cis, ObfPoiRegionSection* section, bool readCategories )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndPoiIndex::kNameFieldNumber:
            cis->ReadString(&section->_name, std::numeric_limits<int>::max());
            break;
        case OsmAndPoiIndex::kBoundariesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readPoiBoundariesIndex(cis, section);
                cis->PopLimit(oldLimit);
            }
            break; 
        case OsmAndPoiIndex::kCategoriesTableFieldNumber:
            {
                if(!readCategories)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readCategory(cis, section);
                cis->PopLimit(oldLimit);
            }
            break;
        case OsmAndPoiIndex::kBoxesFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiRegionSection::readPoiBoundariesIndex( gpb::io::CodedInputStream* cis, ObfPoiRegionSection* section )
{
    gpb::uint32 value;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndTileBox::kLeftFieldNumber:
            cis->ReadVarint32(&value);
            //TODO:section->_leftLongitude = MapUtils.get31LongitudeX(value);
            break;
        case OsmAndTileBox::kRightFieldNumber:
            cis->ReadVarint32(&value);
            //TODO:section->_rightLongitude = MapUtils.get31LongitudeX(value);
            break;
        case OsmAndTileBox::kTopFieldNumber:
            cis->ReadVarint32(&value);
            //TODO:section->_topLatitude = MapUtils.get31LatitudeY(value);
            break;
        case OsmAndTileBox::kBottomFieldNumber:
            cis->ReadVarint32(&value);
            //TODO:section->_bottomLatitude = MapUtils.get31LatitudeY(value);
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiRegionSection::readCategory( gpb::io::CodedInputStream* cis, ObfPoiRegionSection* section )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndCategoryTable::kCategoryFieldNumber:
            {
                std::string category;
                cis->ReadString(&category, std::numeric_limits<int>::max());
                section->_categories.push_back(category);
                //TODO:region.categoriesType.add(AmenityType.fromString(cat));
                section->_subcategories.push_back(std::list<std::string>());
            }
            break;
        case OsmAndCategoryTable::kSubcategoriesFieldNumber:
            {
                std::string category;
                cis->ReadString(&category, std::numeric_limits<int>::max());
                section->_subcategories.back().push_back(category);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

