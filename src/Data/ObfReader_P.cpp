#include "ObfReader_P.h"
#include "ObfReader.h"

#include "QtExtensions.h"
#include <QFile>

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "QIODeviceInputStream.h"
#include "QFileDeviceInputStream.h"
#include "ObfFile.h"
#include "ObfFile_P.h"
#include "ObfInfo.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionReader_P.h"
#include "ObfAddressSectionInfo.h"
#include "ObfAddressSectionReader_P.h"
#include "ObfTransportSectionInfo.h"
#include "ObfTransportSectionReader_P.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionReader_P.h"
#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionReader_P.h"
#include "ObfReaderUtilities.h"
#include "Logging.h"
#include <google/protobuf/wire_format_lite.h>
#include "google/protobuf/wire_format_lite.cc"

//#define OSMAND_TRACE_OBF_READERS 1
#if !defined(OSMAND_TRACE_OBF_READERS)
#   define OSMAND_TRACE_OBF_READERS 0
#endif // !defined(OSMAND_TRACE_OBF_READERS)

using google::protobuf::internal::WireFormatLite;

OsmAnd::ObfReader_P::ObfReader_P(
    ObfReader* const owner_,
    const std::shared_ptr<QIODevice>& input_)
    : _input(input_)
#if OSMAND_VERIFY_OBF_READER_THREAD
    , _threadId(QThread::currentThreadId())
#endif // OSMAND_VERIFY_OBF_READER_THREAD
    , owner(owner_)
{
}

OsmAnd::ObfReader_P::~ObfReader_P()
{
}

bool OsmAnd::ObfReader_P::isOpened() const
{
    return static_cast<bool>(_codedInputStream);
}

bool OsmAnd::ObfReader_P::open()
{
#if OSMAND_VERIFY_OBF_READER_THREAD
    if (_threadId != QThread::currentThreadId())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "ObfReader(%p) was accessed from thread %p, but created in thread %p",
            owner.get(),
            QThread::currentThreadId(),
            _threadId);
#   if OSMAND_VERIFY_OBF_READER_THREAD > 1
        assert(false);
#   endif // OSMAND_VERIFY_OBF_READER_THREAD > 1
    }
#endif // OSMAND_VERIFY_OBF_READER_THREAD

    if (isOpened())
        return false;

    // Create zero-copy input stream
    gpb::io::ZeroCopyInputStream* zcis = nullptr;
    if (const auto inputFileDevice = std::dynamic_pointer_cast<QFileDevice>(_input))
        zcis = new QFileDeviceInputStream(inputFileDevice);
    else
        zcis = new QIODeviceInputStream(_input);
    _zeroCopyInputStream.reset(zcis);

    // Create coded input stream wrapper
    const auto cis = new gpb::io::CodedInputStream(zcis);
    cis->SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    _codedInputStream.reset(cis);

#if OSMAND_TRACE_OBF_READERS
    if (const auto inputFileDevice = std::dynamic_pointer_cast<QFileDevice>(_input))
    {
        LogPrintf(LogSeverityLevel::Debug,
            "Opened ObfReader(%p) in %p for '%s', handle 0x%08x",
            owner.get(),
            QThread::currentThreadId(),
            qPrintable(inputFileDevice->fileName()),
            inputFileDevice->handle());
    }
#endif // OSMAND_TRACE_OBF_READERS

    return true;
}

bool OsmAnd::ObfReader_P::close()
{
#if OSMAND_VERIFY_OBF_READER_THREAD
    if (_threadId != QThread::currentThreadId())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "ObfReader(%p) was accessed from thread %p, but created in thread %p",
            owner.get(),
            QThread::currentThreadId(),
            _threadId);
#   if OSMAND_VERIFY_OBF_READER_THREAD > 1
        assert(false);
#   endif // OSMAND_VERIFY_OBF_READER_THREAD > 1
    }
#endif // OSMAND_VERIFY_OBF_READER_THREAD

    if (!isOpened())
        return false;

#if OSMAND_TRACE_OBF_READERS
    if (const auto fileDeviceInputStream = std::dynamic_pointer_cast<QFileDeviceInputStream>(_zeroCopyInputStream))
    {
        const auto& inputFileDevice = fileDeviceInputStream->file;

        LogPrintf(LogSeverityLevel::Debug,
            "Closing ObfReader(%p) in %p for '%s', handle 0x%08x",
            owner.get(),
            QThread::currentThreadId(),
            qPrintable(inputFileDevice->fileName()),
            inputFileDevice->handle());
    }
#endif // OSMAND_TRACE_OBF_READERS

    _codedInputStream.reset();
    _zeroCopyInputStream.reset();

    return true;
}

std::shared_ptr<const OsmAnd::ObfInfo> OsmAnd::ObfReader_P::obtainInfo() const
{
#if OSMAND_VERIFY_OBF_READER_THREAD
    if (_threadId != QThread::currentThreadId())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "ObfReader(%p) was accessed from thread %p, but created in thread %p",
            owner.get(),
            QThread::currentThreadId(),
            _threadId);
#   if OSMAND_VERIFY_OBF_READER_THREAD > 1
        assert(false);
#   endif // OSMAND_VERIFY_OBF_READER_THREAD > 1
    }
#endif // OSMAND_VERIFY_OBF_READER_THREAD

    // Check if information is already available
    if (_obfInfo)
        return _obfInfo;

    if (!isOpened())
        return nullptr;

    if (owner->obfFile)
    {
        QMutexLocker scopedLocker(&owner->obfFile->_p->_obfInfoMutex);

        if (!owner->obfFile->_p->_obfInfo)
        {
            std::shared_ptr<ObfInfo> obfInfo;
            if (!readInfo(*this, obfInfo))
                return nullptr;
            owner->obfFile->_p->_obfInfo = obfInfo;
        }
        _obfInfo = owner->obfFile->_p->_obfInfo;

        return _obfInfo;
    }
    else
    {
        std::shared_ptr<ObfInfo> obfInfo;
        if (!readInfo(*this, obfInfo))
            return nullptr;
        _obfInfo = obfInfo;

        return _obfInfo;
    }
}

std::shared_ptr<OsmAnd::gpb::io::CodedInputStream> OsmAnd::ObfReader_P::getCodedInputStream() const
{
#if OSMAND_VERIFY_OBF_READER_THREAD
    if (_threadId != QThread::currentThreadId())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "ObfReader(%p) was accessed from thread %p, but created in thread %p",
            owner.get(),
            QThread::currentThreadId(),
            _threadId);
#   if OSMAND_VERIFY_OBF_READER_THREAD > 1
        assert(false);
#   endif // OSMAND_VERIFY_OBF_READER_THREAD > 1
    }
#endif // OSMAND_VERIFY_OBF_READER_THREAD

    return _codedInputStream;
}

bool OsmAnd::ObfReader_P::readInfo(const ObfReader_P& reader, std::shared_ptr<ObfInfo>& outInfo)
{
    const auto cis = reader.getCodedInputStream().get();

    const std::shared_ptr<ObfInfo> info(new ObfInfo());
    bool loadedCorrectly = false;
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return false;

                if (loadedCorrectly)
                {
                    info->calculateCenterPointForRegions();
                    outInfo = info;
                }
                return loadedCorrectly;
            case OBF::OsmAndStructure::kVersionFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&info->version));
                break;
            case OBF::OsmAndStructure::kDateCreatedFieldNumber:
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&info->creationTimestamp));
                break;
            case OBF::OsmAndStructure::kOwnerFieldNumber:
            {
                int len = 0;
                WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(cis, &len);
                const auto oldLimit = cis->PushLimit(len);
                readOsmAndOwner(cis, info);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndStructure::kMapIndexFieldNumber:
            {
                std::shared_ptr<ObfMapSectionInfo> section(new ObfMapSectionInfo(info));
                section->length = ObfReaderUtilities::readBigEndianInt(cis);
                section->offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(section->length);

                ObfMapSectionReader_P::read(reader, section);

                info->isBasemap = info->isBasemap || section->isBasemap;
                info->isBasemapWithCoastlines = info->isBasemapWithCoastlines || section->isBasemapWithCoastlines;
                info->isContourLines = info->isContourLines || section->isContourLines;

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                cis->Seek(section->offset + section->length);

                info->mapSections.push_back(section);
                break;
            }
            case OBF::OsmAndStructure::kAddressIndexFieldNumber:
            {
                const std::shared_ptr<ObfAddressSectionInfo> section(new ObfAddressSectionInfo(info));
                section->length = ObfReaderUtilities::readBigEndianInt(cis);
                section->offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(section->length);

                ObfAddressSectionReader_P::read(reader, section);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                cis->Seek(section->offset + section->length);

                info->addressSections.push_back(section);
                break;
            }
            case OBF::OsmAndStructure::kTransportIndexFieldNumber:
            {
                const std::shared_ptr<ObfTransportSectionInfo> section(new ObfTransportSectionInfo(info));
                section->length = ObfReaderUtilities::readBigEndianInt(cis);
                section->offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(section->length);

                ObfTransportSectionReader_P::read(reader, section);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                cis->Seek(section->offset + section->length);

                info->transportSections.push_back(section);
                break;
            }
            case OBF::OsmAndStructure::kRoutingIndexFieldNumber:
            {
                const std::shared_ptr<ObfRoutingSectionInfo> section(new ObfRoutingSectionInfo(info));
                section->length = ObfReaderUtilities::readBigEndianInt(cis);
                section->offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(section->length);

                ObfRoutingSectionReader_P::read(reader, section);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                cis->Seek(section->offset + section->length);

                info->routingSections.push_back(section);
                break;
            }
            case OBF::OsmAndStructure::kPoiIndexFieldNumber:
            {
                const std::shared_ptr<ObfPoiSectionInfo> section(new ObfPoiSectionInfo(info));
                section->length = ObfReaderUtilities::readBigEndianInt(cis);
                section->offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(section->length);

                ObfPoiSectionReader_P::read(reader, section);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                cis->Seek(section->offset + section->length);

                info->poiSections.push_back(section);
                break;
            }
            case OBF::OsmAndStructure::kVersionConfirmFieldNumber:
            {
                gpb::uint32 controlVersion;
                cis->ReadVarint32(&controlVersion);
                loadedCorrectly = (controlVersion == info->version);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }

    return false;
}

bool OsmAnd::ObfReader_P::readOsmAndOwner(gpb::io::CodedInputStream* cis, const std::shared_ptr<ObfInfo> info)
{
    uint32_t tag;
    while ((tag = cis->ReadTag()) != 0)
    {
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case OBF::OsmAndOwner::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, info->owner.name);
                break;
            case OBF::OsmAndOwner::kResourceFieldNumber:
                ObfReaderUtilities::readQString(cis, info->owner.resource);
                break;
            case OBF::OsmAndOwner::kPluginidFieldNumber:
                ObfReaderUtilities::readQString(cis, info->owner.pluginid);
                break;
            case OBF::OsmAndOwner::kDescriptionFieldNumber:
                ObfReaderUtilities::readQString(cis, info->owner.description);
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
    return true;
}
