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

OsmAnd::ObfReader_P::ObfReader_P(ObfReader* const owner_, const std::shared_ptr<QIODevice>& input_)
    : owner(owner_)
    , _input(input_)
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
    if (isOpened())
        return false;

    // Create zero-copy input stream
    gpb::io::ZeroCopyInputStream* zcis = nullptr;
    if (const auto inputAsFileDevice = std::dynamic_pointer_cast<QFileDevice>(_input))
        zcis = new QFileDeviceInputStream(inputAsFileDevice);
    else
        zcis = new QIODeviceInputStream(_input);
    _zeroCopyInputStream.reset(zcis);

    // Create coded input stream wrapper
    const auto cis = new gpb::io::CodedInputStream(zcis);
    cis->SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    _codedInputStream.reset(cis);

    return true;
}

bool OsmAnd::ObfReader_P::close()
{
    if (!isOpened())
        return false;

    _codedInputStream.reset();
    _zeroCopyInputStream.reset();

    return true;
}

std::shared_ptr<const OsmAnd::ObfInfo> OsmAnd::ObfReader_P::obtainInfo() const
{
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
            if (!readInfo(*this, owner->obfFile->_p->_obfInfo))
                return nullptr;
        }
        _obfInfo = owner->obfFile->_p->_obfInfo;

        return _obfInfo;
    }
    else
    {
        if (!readInfo(*this, _obfInfo))
            return nullptr;

        return _obfInfo;
    }
}

bool OsmAnd::ObfReader_P::readInfo(const ObfReader_P& reader, std::shared_ptr<const ObfInfo>& info_)
{
    auto cis = reader._codedInputStream.get();

    std::shared_ptr<ObfInfo> info(new ObfInfo());
    bool loadedCorrectly = false;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if (loadedCorrectly)
                info_ = info;

            return loadedCorrectly;
        case OBF::OsmAndStructure::kVersionFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&info->_version));
            break;
        case OBF::OsmAndStructure::kDateCreatedFieldNumber:
            cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&info->_creationTimestamp));
            break;
        case OBF::OsmAndStructure::kMapIndexFieldNumber:
            {
                const std::shared_ptr<ObfMapSectionInfo> section(new ObfMapSectionInfo(info));
                section->_length = ObfReaderUtilities::readBigEndianInt(cis);
                section->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_length);

                ObfMapSectionReader_P::read(reader, section);

                info->_isBasemap = info->_isBasemap || section->isBasemap;
                cis->PopLimit(oldLimit);
                cis->Seek(section->_offset + section->_length);

                info->_mapSections.push_back(qMove(section));
            }
            break;
        case OBF::OsmAndStructure::kAddressIndexFieldNumber:
            {
                const std::shared_ptr<ObfAddressSectionInfo> section(new ObfAddressSectionInfo(info));
                section->_length = ObfReaderUtilities::readBigEndianInt(cis);
                section->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_length);

                ObfAddressSectionReader_P::read(reader, section);

                cis->PopLimit(oldLimit);
                cis->Seek(section->_offset + section->_length);

                info->_addressSections.push_back(qMove(section));
            }
            break;
        case OBF::OsmAndStructure::kTransportIndexFieldNumber:
            {
                const std::shared_ptr<ObfTransportSectionInfo> section(new ObfTransportSectionInfo(info));
                section->_length = ObfReaderUtilities::readBigEndianInt(cis);
                section->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_length);

                ObfTransportSectionReader_P::read(reader, section);

                cis->PopLimit(oldLimit);
                cis->Seek(section->_offset + section->_length);

                info->_transportSections.push_back(qMove(section));
            }
            break;
        case OBF::OsmAndStructure::kRoutingIndexFieldNumber:
            {
                const std::shared_ptr<ObfRoutingSectionInfo> section(new ObfRoutingSectionInfo(info));
                section->_length = ObfReaderUtilities::readBigEndianInt(cis);
                section->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_length);

                ObfRoutingSectionReader_P::read(reader, section);

                cis->PopLimit(oldLimit);
                cis->Seek(section->_offset + section->_length);

                info->_routingSections.push_back(qMove(section));
            }
            break;
        case OBF::OsmAndStructure::kPoiIndexFieldNumber:
            {
                const std::shared_ptr<ObfPoiSectionInfo> section(new ObfPoiSectionInfo(info));
                section->_length = ObfReaderUtilities::readBigEndianInt(cis);
                section->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_length);

                ObfPoiSectionReader_P::read(reader, section);

                cis->PopLimit(oldLimit);
                cis->Seek(section->_offset + section->_length);

                info->_poiSections.push_back(qMove(section));
            }
            break;
        case OBF::OsmAndStructure::kVersionConfirmFieldNumber:
            {
                gpb::uint32 controlVersion;
                cis->ReadVarint32(&controlVersion);
                loadedCorrectly = (controlVersion == info->_version);
                if (!loadedCorrectly)
                    break;
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }

    return false;
}
