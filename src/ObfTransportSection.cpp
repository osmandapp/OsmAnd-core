#include "ObfTransportSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

OsmAnd::ObfTransportSection::ObfTransportSection()
    : _top(0)
    , _left(0)
    , _bottom(0)
    , _right(0)
    , _stopsFileOffset(0)
    , _stopsFileLength(0)
{
}

OsmAnd::ObfTransportSection::~ObfTransportSection()
{
}

void OsmAnd::ObfTransportSection::read( gpb::io::CodedInputStream* cis, ObfTransportSection* section )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndTransportIndex::kRoutesFieldNumber:
            ObfReader::skipUnknownField(cis, tag);
            break;
        case OBF::OsmAndTransportIndex::kNameFieldNumber:
            gpb::internal::WireFormatLite::ReadString(cis, &section->_name);
            break;
        case OBF::OsmAndTransportIndex::kStopsFieldNumber:
            {
                section->_stopsFileLength = ObfReader::readBigEndianInt(cis);
                section->_stopsFileOffset = cis->TotalBytesRead();
                auto oldLimit = cis->PushLimit(section->_stopsFileLength);
                readTransportBounds(cis, section);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndTransportIndex::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto offset = cis->TotalBytesRead();
                cis->Seek(offset + length);
            }
            //TODO:
            /*IndexStringTable st = new IndexStringTable();
            st.length = codedIS.readRawVarint32();
            st.fileOffset = codedIS.getTotalBytesRead();
            // Do not cache for now save memory
            // readStringTable(st, 0, 20, true);
            ind.stringTable = st;
            codedIS.seek(st.length + st.fileOffset);*/
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfTransportSection::readTransportBounds( gpb::io::CodedInputStream* cis, ObfTransportSection* section )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::TransportStopsTree::kLeftFieldNumber:
            section->_left = ObfReader::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kRightFieldNumber:
            section->_right = ObfReader::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kTopFieldNumber:
            section->_top = ObfReader::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kBottomFieldNumber:
            section->_bottom = ObfReader::readSInt32(cis);
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

