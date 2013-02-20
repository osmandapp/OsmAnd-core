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

    bool loadedCorrectly = false;
    for(;;)
    {
        auto tag = _codedInputStream->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(!loadedCorrectly)
                throw new std::invalid_argument("Corrupted file. It should be ended as it starts with version");
            return;
        case OsmAndStructure::kVersionFieldNumber:
            _codedInputStream->ReadVarint32(reinterpret_cast<gpb::uint32*>(&_version));
            break;
        case OsmAndStructure::kDateCreatedFieldNumber:
            _codedInputStream->ReadVarint64(reinterpret_cast<gpb::uint64*>(&_creationTimestamp));
            break;
        case OsmAndStructure::kMapIndexFieldNumber:
            {
                std::shared_ptr<ObfMapSection> section(new ObfMapSection());
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfMapSection::read(_codedInputStream.get(), section.get(), false);
                _isBasemap = _isBasemap || section->isBaseMap();
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _mapSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OsmAndStructure::kAddressIndexFieldNumber:
            {
                std::shared_ptr<ObfAddressSection> section(new ObfAddressSection());
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfAddressSection::read(_codedInputStream.get(), section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _addressRegionsSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OsmAndStructure::kTransportIndexFieldNumber:
            {
                std::shared_ptr<ObfTransportSection> section(new ObfTransportSection());
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfTransportSection::read(_codedInputStream.get(), section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _transportSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OsmAndStructure::kRoutingIndexFieldNumber:
            {
                std::shared_ptr<ObfRoutingSection> section(new ObfRoutingSection());
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfRoutingSection::read(_codedInputStream.get(), section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _routingRegionsSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OsmAndStructure::kPoiIndexFieldNumber:
            {
                std::shared_ptr<ObfPoiSection> section(new ObfPoiSection());
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->TotalBytesRead();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfPoiSection::read(_codedInputStream.get(), section.get(), false);
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _poiRegionsSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OsmAndStructure::kVersionConfirmFieldNumber:
            {
                gpb::uint32 controlVersion;
                _codedInputStream->ReadVarint32(&controlVersion);
                loadedCorrectly = (controlVersion == _version);
                if(!loadedCorrectly)
                    break;
                //TODO:calculateCenterPointForRegions();
            }
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
    if(wireType == gpb::internal::WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED)
    {
        gpb::uint32 length;
        if(cis->ReadRaw(&length, sizeof(length)))
        {
            auto nativeLength = qFromBigEndian(length);
            cis->Skip(nativeLength);
        }
    }
    else
        gpb::internal::WireFormatLite::SkipField(cis, tag);
}

int OsmAnd::ObfReader::readBigEndianInt( gpb::io::CodedInputStream* cis )
{
    gpb::uint32 be;
    cis->ReadRaw(&be, sizeof(be));
    auto ne = qFromBigEndian(be);
    return ne;
}

int OsmAnd::ObfReader::getVersion() const
{
    return _version;
}

std::list< OsmAnd::ObfSection* > OsmAnd::ObfReader::getSections() const
{
    return _sections;
}

int OsmAnd::ObfReader::readSInt32( gpb::io::CodedInputStream* cis )
{
    gpb::uint32 value;
    cis->ReadVarint32(&value);
    return gpb::internal::WireFormatLite::ZigZagDecode32(value);
}
