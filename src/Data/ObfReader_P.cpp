#include "ObfReader_P.h"

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

#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>

OsmAnd::ObfReader_P::ObfReader_P( ObfReader* owner_ )
    : owner(owner_)
{
}

OsmAnd::ObfReader_P::~ObfReader_P()
{
    _codedInputStream.reset();
    _zeroCopyInputStream.reset();

}

void OsmAnd::ObfReader_P::readInfo( const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfInfo>& info )
{
    auto cis = reader->_codedInputStream.get();

    bool loadedCorrectly = false;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(!loadedCorrectly)
                info->_version = -1;
            return;
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
                if(!loadedCorrectly)
                    break;
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}
