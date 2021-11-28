#include "ObfReaderUtilities.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QtEndian>
#include <QThread>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfSectionInfo.h"
#include "Logging.h"
#include "CollatorStringMatcher.h"

bool OsmAnd::ObfReaderUtilities::readQString(gpb::io::CodedInputStream* cis, QString& output)
{
    std::string value;
    if (!gpb::internal::WireFormatLite::ReadString(cis, &value))
        return false;

    output = QString::fromStdString(value);
    return true;
}

int32_t OsmAnd::ObfReaderUtilities::readSInt32(gpb::io::CodedInputStream* cis)
{
    gpb::uint32 value;
    cis->ReadVarint32(&value);

    auto decodedValue = gpb::internal::WireFormatLite::ZigZagDecode32(value);
    return decodedValue;
}

int64_t OsmAnd::ObfReaderUtilities::readSInt64(gpb::io::CodedInputStream* cis)
{
    gpb::uint64 value;
    cis->ReadVarint64(&value);

    auto decodedValue = gpb::internal::WireFormatLite::ZigZagDecode64(value);
    return decodedValue;
}

uint32_t OsmAnd::ObfReaderUtilities::readBigEndianInt(gpb::io::CodedInputStream* cis)
{
    gpb::uint32 be;
    cis->ReadRaw(&be, sizeof(be));
    auto ne = qFromBigEndian(be);
    return ne;
}

uint32_t OsmAnd::ObfReaderUtilities::readLength(gpb::io::CodedInputStream* cis)
{
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    return length;
}

void OsmAnd::ObfReaderUtilities::readStringTable(gpb::io::CodedInputStream* cis, QStringList& stringTableOut)
{
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::StringTable::kSFieldNumber:
            {
                QString value;
                if (readQString(cis, value))
                    stringTableOut.push_back(qMove(value));
                break;
            }
            default:
                skipUnknownField(cis, tag);
                break;
        }
    }
}

int OsmAnd::ObfReaderUtilities::scanIndexedStringTable(
    gpb::io::CodedInputStream* cis,
    const QString& query,
    QVector<uint32_t>& outValues,
    const bool strictMatch /*= false*/,
    const QString& keysPrefix /*= QString::null*/,
    const int matchedCharactersCount_ /*= 0*/)
{
    QString key;
    auto matchedCharactersCount = matchedCharactersCount_;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return matchedCharactersCount;

                return matchedCharactersCount;
            case OBF::IndexedStringTable::kKeyFieldNumber:
            {
                readQString(cis, key);
                if (!keysPrefix.isEmpty())
                    key.prepend(keysPrefix);

                bool matchesForward = false;
                bool matchesBackward = false;
                if (strictMatch)
                    matchesForward = key.startsWith(query, Qt::CaseInsensitive);
                else
                    matchesForward = CollatorStringMatcher::cmatches(key, query, StringMatcherMode::CHECK_ONLY_STARTS_WITH);

                if (!matchesForward)
                {
                    if (strictMatch)
                        matchesBackward = query.startsWith(key, Qt::CaseInsensitive);
                    else
                        matchesBackward = CollatorStringMatcher::cmatches(query, key, StringMatcherMode::CHECK_ONLY_STARTS_WITH);
                }

                if (matchesForward)
                {
                    if (query.length() > matchedCharactersCount)
                    {
                        matchedCharactersCount = query.length();
                        outValues.clear();
                    }
                    else if (query.length() < matchedCharactersCount)
                    {
                        key = QString();
                    }
                }
                else if (matchesBackward)
                {
                    if (key.length() > matchedCharactersCount)
                    {
                        matchedCharactersCount = key.length();
                        outValues.clear();
                    }
                    else if (key.length() < matchedCharactersCount)
                    {
                        key = QString();
                    }
                }
                else
                {
                    key = QString();
                }
                break;
            }
            case OBF::IndexedStringTable::kValFieldNumber:
            {
                const auto value = readBigEndianInt(cis);

                if (!key.isNull())
                    outValues.push_back(value);
                break;
            }
            case OBF::IndexedStringTable::kSubtablesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readLength(cis);
                const auto oldLimit = cis->PushLimit(length);

                if (!key.isNull())
                    matchedCharactersCount = scanIndexedStringTable(cis, query, outValues, strictMatch, key, matchedCharactersCount);
                else
                    cis->Skip(cis->BytesUntilLimit());

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                break;
            }
            default:
                skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfReaderUtilities::readTileBox(gpb::io::CodedInputStream* cis, AreaI& outArea)
{
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndTileBox::kLeftFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&outArea.left()));
                break;
            case OBF::OsmAndTileBox::kRightFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&outArea.right()));
                break;
            case OBF::OsmAndTileBox::kTopFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&outArea.top()));
                break;
            case OBF::OsmAndTileBox::kBottomFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&outArea.bottom()));
                break;
            default:
                skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfReaderUtilities::skipUnknownField(gpb::io::CodedInputStream* cis, int tag)
{
    const auto wireType = gpb::internal::WireFormatLite::GetTagWireType(tag);
    if (wireType == gpb::internal::WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED)
    {
        const auto length = readBigEndianInt(cis);
        cis->Skip(length);
        return;
    }
    
    gpb::internal::WireFormatLite::SkipField(cis, tag);
}

void OsmAnd::ObfReaderUtilities::skipBlockWithLength(gpb::io::CodedInputStream* cis)
{
    cis->Skip(readLength(cis));
}

QString OsmAnd::ObfReaderUtilities::encodeIntegerToString(const uint32_t value)
{
    QString fakeQString(2, QChar(QChar::Null));
    fakeQString.data()[0].unicode() = static_cast<ushort>((value >> 16 * 0) & 0xffff);
    fakeQString.data()[1].unicode() = static_cast<ushort>((value >> 16 * 1) & 0xffff);

    assert(decodeIntegerFromString(fakeQString) == value);

    return fakeQString;
}

uint32_t OsmAnd::ObfReaderUtilities::decodeIntegerFromString(const QString& container)
{
    uint32_t res = 0;

    ushort value;

    value = container.at(0).unicode();
    res |= (value & 0xffff) << 16 * 0;

    value = container.at(1).unicode();
    res |= (value & 0xffff) << 16 * 1;

    return res;
}

bool OsmAnd::ObfReaderUtilities::reachedDataEnd(gpb::io::CodedInputStream* cis)
{
    if (cis->ConsumedEntireMessage())
        return true;

    LogPrintf(LogSeverityLevel::Warning,
        "Unexpected data end at %d, %d byte(s) not read",
        cis->CurrentPosition(),
        cis->BytesUntilLimit());

    return false;
}

void OsmAnd::ObfReaderUtilities::ensureAllDataWasRead(gpb::io::CodedInputStream* cis)
{
    const auto bytesUntilLimit = cis->BytesUntilLimit();
    if (bytesUntilLimit == 0)
        return;

    LogPrintf(LogSeverityLevel::Warning,
        "Unexpected %d unread byte(s) at %d",
        cis->BytesUntilLimit(),
        cis->CurrentPosition());

    cis->Skip(cis->BytesUntilLimit());
}
