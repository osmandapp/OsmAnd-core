#include "ObfAddressRegionSection.h"

#include <QtEndian>
#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "../native/src/proto/osmand_odb.pb.h"

OsmAnd::ObfAddressRegionSection::ObfAddressRegionSection()
    : _indexNameOffset(-1)
{
}

void OsmAnd::ObfAddressRegionSection::read( gpb::io::CodedInputStream* cis, ObfAddressRegionSection* section )
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
            cis->ReadString(&section->_name, std::numeric_limits<int>::max());
            break;
        case OsmAndAddressIndex::kNameEnFieldNumber:
            cis->ReadString(&section->_enName, std::numeric_limits<int>::max());
            break;
        case OsmAndAddressIndex::kCitiesFieldNumber:
            {
                std::shared_ptr<CitiesBlock> citiesBlock(new CitiesBlock());
                section->_cities.push_back(citiesBlock);

                citiesBlock->_type = 1;//TODO: Victor, what this 1 means?
                gpb::uint32 length;
                cis->ReadRaw(&length, sizeof(length));
                section->_length = qFromBigEndian(length);
                section->_offset = cis->TotalBytesRead();

                for(;;)
                {
                    auto tag2 = cis->ReadTag();
                    auto tagFieldNum2 = gpb::internal::WireFormatLite::GetTagFieldNumber(tag2);
                    if(!tagFieldNum2)
                        break;

                    switch(tagFieldNum2)
                    {
                    case OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber:
                        cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&citiesBlock->_type));
                    default:
                        ObfReader::skipUnknownField(cis, tag);
                        break;
                    }
                }

                cis->Seek(section->_offset + section->_length);
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
