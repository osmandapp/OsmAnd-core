#include "ObfAddressSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

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
        case OBF::OsmAndAddressIndex::kNameFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_name);
            break;
        case OBF::OsmAndAddressIndex::kNameEnFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_enName);
            break;
        case OBF::OsmAndAddressIndex::kCitiesFieldNumber:
            {
                std::shared_ptr<CitiesBlock> entry(new CitiesBlock());
                entry->_length = ObfReader::readBigEndianInt(cis);
                entry->_offset = cis->TotalBytesRead();
                CitiesBlock::read(cis, entry.get());
                cis->Seek(entry->_offset + entry->_length);
                section->_entries.push_back(entry);
            }
            break;
        case OBF::OsmAndAddressIndex::kNameIndexFieldNumber:
            {
                section->_indexNameOffset = cis->TotalBytesRead();
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
/*
void OsmAnd::ObfAddressSection::readAllEntriesOfType( ObfReader* reader, ObfAddressSection* section, Entry::Type * type )
{

}
*/
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
        case OBF::OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_type));
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}
