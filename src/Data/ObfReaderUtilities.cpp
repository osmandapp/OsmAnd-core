#include "ObfReaderUtilities.h"

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

QString OsmAnd::ObfReaderUtilities::encodeIntegerToString( const uint32_t& value )
{
    QChar fakeString[] =
    {
        QChar('['),
        QChar(0x0001 + static_cast<ushort>((value >> 8*0) & 0xff)),
        QChar(0x0001 + static_cast<ushort>((value >> 8*1) & 0xff)),
        QChar(0x0001 + static_cast<ushort>((value >> 8*2) & 0xff)),
        QChar(0x0001 + static_cast<ushort>((value >> 8*3) & 0xff)),
        QChar(']')
    };
    const QString fakeQString(fakeString);

    assert(fakeQString.length() == 36);
    assert(decodeIntegerFromString(fakeQString) == value);

    return fakeQString;
}

uint32_t OsmAnd::ObfReaderUtilities::decodeIntegerFromString( const QString& container )
{
    uint32_t res = 0;

    assert(container.length() == 36);

    ushort value;

    value = container.at(1 + 0).unicode() - 0x0001;
    res |= (value & 0xff) << 8*0;

    value = container.at(1 + 1).unicode() - 0x0001;
    res |= (value & 0xff) << 8*1;

    value = container.at(1 + 2).unicode() - 0x0001;
    res |= (value & 0xff) << 8*2;

    value = container.at(1 + 3).unicode() - 0x0001;
    res |= (value & 0xff) << 8*3;

    return res;
}

