#include "ObfReaderUtilities.h"

#include <OsmAndCore/QtExtensions.h>
#include <QtEndian>

#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

bool OsmAnd::ObfReaderUtilities::readQString( gpb::io::CodedInputStream* cis, QString& output )
{
    std::string value;
    if(!gpb::internal::WireFormatLite::ReadString(cis, &value))
        return false;

    output = QString::fromUtf8(value.c_str(), value.size());
    return true;
}

int32_t OsmAnd::ObfReaderUtilities::readSInt32( gpb::io::CodedInputStream* cis )
{
    gpb::uint32 value;
    cis->ReadVarint32(&value);

    auto decodedValue = gpb::internal::WireFormatLite::ZigZagDecode32(value);
    return decodedValue;
}

int64_t OsmAnd::ObfReaderUtilities::readSInt64( gpb::io::CodedInputStream* cis )
{
    gpb::uint64 value;
    cis->ReadVarint64(&value);

    auto decodedValue = gpb::internal::WireFormatLite::ZigZagDecode64(value);
    return decodedValue;
}

uint32_t OsmAnd::ObfReaderUtilities::readBigEndianInt( gpb::io::CodedInputStream* cis )
{
    gpb::uint32 be;
    cis->ReadRaw(&be, sizeof(be));
    auto ne = qFromBigEndian(be);
    return ne;
}

void OsmAnd::ObfReaderUtilities::readStringTable( gpb::io::CodedInputStream* cis, QStringList& stringTableOut )
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

void OsmAnd::ObfReaderUtilities::skipUnknownField( gpb::io::CodedInputStream* cis, int tag )
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

QString OsmAnd::ObfReaderUtilities::encodeIntegerToString( const uint32_t value )
{
    QString fakeQString(2, QChar(QChar::Null));
    fakeQString.data()[0].unicode() = static_cast<ushort>((value >> 16*0) & 0xffff);
    fakeQString.data()[1].unicode() = static_cast<ushort>((value >> 16*1) & 0xffff);

    assert(decodeIntegerFromString(fakeQString) == value);

    return fakeQString;
}

uint32_t OsmAnd::ObfReaderUtilities::decodeIntegerFromString( const QString& container )
{
    uint32_t res = 0;

    ushort value;

    value = container.at(0).unicode();
    res |= (value & 0xffff) << 16*0;

    value = container.at(1).unicode();
    res |= (value & 0xffff) << 16*1;

    return res;
}

