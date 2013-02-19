#include "ObfReader.h"

#include <QZeroCopyInputStream.h>
#include <google/protobuf/wire_format_lite.h>
#include <QtEndian>

#include "../native/src/proto/osmand_odb.pb.h"

namespace gpb = google::protobuf;

OsmAnd::ObfReader::ObfReader( QIODevice* input )
    : _codedInputStream(new gpb::io::CodedInputStream(new QZeroCopyInputStream(input)))
    , _isBasemap(false)
{
    _codedInputStream->SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());

    gpb::uint32 tag;
    while ((tag = _codedInputStream->ReadTag()) != 0)
    {
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            throw new std::exception(); // Corrupted file. It should be ended as it starts with version
        case OsmAndStructure::kVersionFieldNumber:
            _codedInputStream->ReadVarint32(reinterpret_cast<gpb::uint32*>(&_version));
            break;
        case OsmAndStructure::kDateCreatedFieldNumber:
            _codedInputStream->ReadVarint64(reinterpret_cast<gpb::uint64*>(&_creationTimestamp));
            break;
        case OsmAndStructure::kMapIndexFieldNumber:
            {
                std::shared_ptr<ObfMapSection> section(new ObfMapSection());
                gpb::uint32 length;
                _codedInputStream->ReadRaw(&length, sizeof(length));
                section->_length = qFromBigEndian(length);
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfMapSection::read(_codedInputStream.get(), section.get(), false);
                _isBasemap = _isBasemap || section->isBaseMap();
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _mapSections.push_back(section);
                //TODO:indexes.add(mapIndex);
            }
            break;
        case OsmAndStructure::kAddressIndexFieldNumber:
            {
                std::shared_ptr<ObfAddressRegionSection> section(new ObfAddressRegionSection());
                gpb::uint32 length;
                _codedInputStream->ReadRaw(&length, sizeof(length));
                section->_length = qFromBigEndian(length);
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfAddressRegionSection::read(_codedInputStream.get(), section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _addressRegionsSections.push_back(section);
                //TODO:indexes.add(mapIndex);
            }
            break;
        case OsmAndStructure::kTransportIndexFieldNumber:
            /*TransportIndex ind = new TransportIndex();
            ind.length = readInt();
            ind.filePointer = codedIS.getTotalBytesRead();
            if (transportAdapter != null) {
            oldLimit = codedIS.pushLimit(ind.length);
            transportAdapter.readTransportIndex(ind);
            codedIS.popLimit(oldLimit);
            transportIndexes.add(ind);
            indexes.add(ind);
            }
            codedIS.seek(ind.filePointer + ind.length);*/
            break;
        case OsmAndStructure::kRoutingIndexFieldNumber:
            {
                std::shared_ptr<ObfRoutingRegionSection> section(new ObfRoutingRegionSection());
                gpb::uint32 length;
                _codedInputStream->ReadRaw(&length, sizeof(length));
                section->_length = qFromBigEndian(length);
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfRoutingRegionSection::read(_codedInputStream.get(), section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _routingRegionsSections.push_back(section);
                //TODO:indexes.add(mapIndex);
            }
            break;
        case OsmAndStructure::kPoiIndexFieldNumber:
            {
                std::shared_ptr<ObfPoiRegionSection> section(new ObfPoiRegionSection());
                gpb::uint32 length;
                _codedInputStream->ReadRaw(&length, sizeof(length));
                section->_length = qFromBigEndian(length);
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfPoiRegionSection::read(_codedInputStream.get(), section.get(), false);
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _poiRegionsSections.push_back(section);
                //TODO:indexes.add(mapIndex);
            }
            break;
        case OsmAndStructure::kVersionConfirmFieldNumber:
            /*int cversion = codedIS.readUInt32();
            calculateCenterPointForRegions();
            initCorrectly = cversion == version;*/
            break;
        default:
            skipUnknownField(_codedInputStream.get(), tag);
            break;
        }
    }
}

void OsmAnd::ObfReader::skipUnknownField( gpb::io::CodedInputStream* cis, int tag )
{
    auto wireType = gpb::internal::WireFormatLite::GetTagWireType(tag);
    if(wireType == gpb::internal::WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED){
        gpb::uint32 length;
        cis->ReadRaw(&length, sizeof(length));
        length = qFromBigEndian(length);
        cis->Skip(length);
    } else {
        gpb::internal::WireFormatLite::SkipField(cis, tag);
    }
}
