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

void OsmAnd::ObfReaderUtilities::readIndexedStringTable(
    gpb::io::CodedInputStream* cis,
    const QList<QString>& queries,
    QList<QVector<uint32_t>>& listOffsets,
    QVector<int>& matchedCharacters,
    const QString& prefix /*= QString::null*/
    )
{
    QString key;
    QVector<bool> matched(matchedCharacters.size());
    bool shouldWeReadSubtable = false;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                return;
                
            case OBF::IndexedStringTable::kKeyFieldNumber:
            {
                readQString(cis, key);
                if (!prefix.isEmpty())
                    key.prepend(prefix);
                
                shouldWeReadSubtable = false;
                for (int i = 0; i < queries.size(); i++)
                {
                    int charMatches = matchedCharacters[i];
                    QString query = queries[i];
                    matched[i] = false;
                    if (query.isNull())
                        continue;
                    
                    // check query is part of key (the best matching)
                    if (CollatorStringMatcher::cmatches(key, query, StringMatcherMode::CHECK_ONLY_STARTS_WITH))
                    {
                        if (query.length() >= charMatches)
                        {
                            if (query.length() > charMatches)
                            {
                                matchedCharacters[i] = query.length();
                                listOffsets[i].clear();
                            }
                            matched[i] = true;
                        }
                    }
                    // check key is part of query
                    else if (CollatorStringMatcher::cmatches(query, key, StringMatcherMode::CHECK_ONLY_STARTS_WITH))
                    {
                        if (key.length() >= charMatches)
                        {
                            if (key.length() > charMatches)
                            {
                                matchedCharacters[i] = key.length();
                                listOffsets[i].clear();
                            }
                            matched[i] = true;
                        }
                    }
                    shouldWeReadSubtable |= matched[i];
                }
                break;
            }
            case OBF::IndexedStringTable::kValFieldNumber:
            {
                const auto value = readBigEndianInt(cis);
                for (int i = 0; i < queries.size(); i++)
                {
                    if (matched[i])
                        listOffsets[i].push_back(value);
                }
                break;
            }
            case OBF::IndexedStringTable::kSubtablesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readLength(cis);
                const auto oldLimit = cis->PushLimit(length);
                
                if (shouldWeReadSubtable && !key.isNull())
                {
                    QList<QString> subqueries = queries;
                    
                    // reset query so we don't search what was not matched
                    for (int i = 0; i < queries.size(); i++)
                    {
                        if (!matched[i])
                            subqueries[i] = QString::null;
                    }
                    readIndexedStringTable(cis, subqueries, listOffsets, matchedCharacters, key);
                }
                else
                {
                    cis->Skip(cis->BytesUntilLimit());
                }

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
