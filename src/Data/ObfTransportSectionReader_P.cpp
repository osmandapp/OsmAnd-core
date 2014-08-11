#include "ObfTransportSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfReader_P.h"
#include "ObfTransportSectionInfo.h"
#include "ObfReaderUtilities.h"
#include "Utilities.h"

OsmAnd::ObfTransportSectionReader_P::ObfTransportSectionReader_P()
{
}

OsmAnd::ObfTransportSectionReader_P::~ObfTransportSectionReader_P()
{
}

void OsmAnd::ObfTransportSectionReader_P::read( const ObfReader_P& reader, const std::shared_ptr<ObfTransportSectionInfo>& section )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndTransportIndex::kRoutesFieldNumber:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        case OBF::OsmAndTransportIndex::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, section->_name);
            break;
        case OBF::OsmAndTransportIndex::kStopsFieldNumber:
            {
                section->_stopsLength = ObfReaderUtilities::readBigEndianInt(cis);
                section->_stopsOffset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_stopsLength);
                readTransportStopsBounds(reader, section);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndTransportIndex::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto offset = cis->CurrentPosition();
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfTransportSectionReader_P::readTransportStopsBounds( const ObfReader_P& reader, const std::shared_ptr<ObfTransportSectionInfo>& section )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::TransportStopsTree::kLeftFieldNumber:
            section->_area24.left() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kRightFieldNumber:
            section->_area24.right() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kTopFieldNumber:
            section->_area24.top() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kBottomFieldNumber:
            section->_area24.bottom() = ObfReaderUtilities::readSInt32(cis);
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}
