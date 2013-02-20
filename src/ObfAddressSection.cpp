#include "ObfAddressSection.h"

#include <QtEndian>
#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "../native/src/proto/osmand_odb.pb.h"

OsmAnd::ObfAddressSection::ObfAddressSection()
    : _indexNameOffset(-1)
{
}

OsmAnd::ObfAddressSection::~ObfAddressSection()
{
}

void OsmAnd::ObfAddressSection::read( gpb::io::CodedInputStream* cis, ObfAddressSection* section )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(section->_enName.empty())
                section->_enName = "Junidecode.unidecode(region.name)";//TODO: this should be fixed
            return;
        case OsmAndAddressIndex::kNameFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_name);
            break;
        case OsmAndAddressIndex::kNameEnFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_enName);
            break;
        case OsmAndAddressIndex::kCitiesFieldNumber:
            {
                std::shared_ptr<CitiesBlock> citiesBlock(new CitiesBlock());
                citiesBlock->_type = 1;//TODO: Victor, what this 1 means?
                gpb::uint32 length;
                cis->ReadRaw(&length, sizeof(length));
                citiesBlock->_length = qFromBigEndian(length);
                citiesBlock->_offset = cis->TotalBytesRead();
                CitiesBlock::read(cis, citiesBlock.get());
                cis->Seek(citiesBlock->_offset + citiesBlock->_length);
                section->_cities.push_back(citiesBlock);
            }
            break;
        case OsmAndAddressIndex::kNameIndexFieldNumber:
            {
                section->_indexNameOffset = cis->TotalBytesRead();
                int length;
                cis->ReadRaw(&length, sizeof(length));
                length = qFromBigEndian(length);
                cis->Seek(section->_indexNameOffset + length + 4);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }

}

OsmAnd::ObfAddressSection::CitiesBlock::CitiesBlock()
{
}

OsmAnd::ObfAddressSection::CitiesBlock::~CitiesBlock()
{
}

void OsmAnd::ObfAddressSection::CitiesBlock::read( gpb::io::CodedInputStream* cis, CitiesBlock* section )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_type));
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}
