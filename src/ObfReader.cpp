#include "ObfReader.h"

#include <stdexcept>

#include "OsmAndLogging.h"

#include "QZeroCopyInputStream.h"
#include <google/protobuf/wire_format_lite.h>
#include <QtEndian>

#include "OBF.pb.h"

namespace gpb = google::protobuf;

OsmAnd::ObfReader::ObfReader( const std::shared_ptr<QIODevice>& input )
    : _codedInputStream(new gpb::io::CodedInputStream(new QZeroCopyInputStream(input)))
    , _isBasemap(false)
    , source(input)
    , version(_version)
    , creationTimestamp(_creationTimestamp)
    , isBaseMap(_isBasemap)
    , mapSections(_mapSections)
    , addressSections(_addressSections)
    , routingSections(_routingSections)
    , poiSections(_poiSections)
    , transportSections(_transportSections)
    , sections(_sections)
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
                throw std::invalid_argument("Corrupted file. It should be ended as it starts with version");
            return;
        case OBF::OsmAndStructure::kVersionFieldNumber:
            _codedInputStream->ReadVarint32(reinterpret_cast<gpb::uint32*>(&_version));
            break;
        case OBF::OsmAndStructure::kDateCreatedFieldNumber:
            _codedInputStream->ReadVarint64(reinterpret_cast<gpb::uint64*>(&_creationTimestamp));
            break;
        case OBF::OsmAndStructure::kMapIndexFieldNumber:
            {
                std::shared_ptr<ObfMapSection> section(new ObfMapSection(this));
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->CurrentPosition();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfMapSection::read(this, section.get());
                _isBasemap = _isBasemap || section->isBaseMap;
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _mapSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OBF::OsmAndStructure::kAddressIndexFieldNumber:
            {
                std::shared_ptr<ObfAddressSection> section(new ObfAddressSection(this));
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->CurrentPosition();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfAddressSection::read(this, section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _addressSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OBF::OsmAndStructure::kTransportIndexFieldNumber:
            {
                std::shared_ptr<ObfTransportSection> section(new ObfTransportSection(this));
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->CurrentPosition();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfTransportSection::read(this, section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _transportSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OBF::OsmAndStructure::kRoutingIndexFieldNumber:
            {
                std::shared_ptr<ObfRoutingSection> section(new ObfRoutingSection(this));
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->CurrentPosition();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfRoutingSection::read(this, section);
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _routingSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OBF::OsmAndStructure::kPoiIndexFieldNumber:
            {
                std::shared_ptr<ObfPoiSection> section(new ObfPoiSection(this));
                section->_length = ObfReader::readBigEndianInt(_codedInputStream.get());
                section->_offset = _codedInputStream->CurrentPosition();
                auto oldLimit = _codedInputStream->PushLimit(section->_length);
                ObfPoiSection::read(this, section.get());
                _codedInputStream->PopLimit(oldLimit);
                _codedInputStream->Seek(section->_offset + section->_length);
                _poiSections.push_back(section);
                _sections.push_back(dynamic_cast<ObfSection*>(section.get()));
            }
            break;
        case OBF::OsmAndStructure::kVersionConfirmFieldNumber:
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

OsmAnd::ObfReader::~ObfReader()
{
}

void OsmAnd::ObfReader::skipUnknownField( gpb::io::CodedInputStream* cis, int tag )
{
    auto wireType = gpb::internal::WireFormatLite::GetTagWireType(tag);
    if(wireType == gpb::internal::WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED)
    {
        auto length = readBigEndianInt(cis);
        cis->Skip(length);
    }
    else
        gpb::internal::WireFormatLite::SkipField(cis, tag);
}

uint32_t OsmAnd::ObfReader::readBigEndianInt( gpb::io::CodedInputStream* cis )
{
    gpb::uint32 be;
    cis->ReadRaw(&be, sizeof(be));
    auto ne = qFromBigEndian(be);
    return ne;
}

bool OsmAnd::ObfReader::readQString( gpb::io::CodedInputStream* cis, QString& output )
{
    std::string value;
    if(!gpb::internal::WireFormatLite::ReadString(cis, &value))
        return false;

    output = QString::fromUtf8(value.c_str(), value.size());
    return true;
}

int32_t OsmAnd::ObfReader::readSInt32( gpb::io::CodedInputStream* cis )
{
    gpb::uint32 value;
    cis->ReadVarint32(&value);

    auto decodedValue = gpb::internal::WireFormatLite::ZigZagDecode32(value);
    return decodedValue;
}

int64_t OsmAnd::ObfReader::readSInt64( gpb::io::CodedInputStream* cis )
{
    gpb::uint64 value;
    cis->ReadVarint64(&value);

    auto decodedValue = gpb::internal::WireFormatLite::ZigZagDecode64(value);
    return decodedValue;
}

QString OsmAnd::ObfReader::transliterate( QString input )
{
    return QString("!translit!");
}

void OsmAnd::ObfReader::readStringTable( gpb::io::CodedInputStream* cis, QStringList& stringTableOut )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::StringTable::kSFieldNumber:
            {
                QString value;
                if(readQString(cis, value))
                    stringTableOut.append(value);
            }
            break;
        default:
            skipUnknownField(cis, tag);
            break;
        }
    }
}
