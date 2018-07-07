#include "ObfTransportSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfReader_P.h"
#include "ObfTransportSectionInfo.h"
#include "TransportStop.h"
#include "ObfReaderUtilities.h"
#include "Utilities.h"

OsmAnd::ObfTransportSectionReader_P::ObfTransportSectionReader_P()
{
}

OsmAnd::ObfTransportSectionReader_P::~ObfTransportSectionReader_P()
{
}

void OsmAnd::ObfTransportSectionReader_P::read(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfTransportSectionInfo>& section)
{
    const auto cis = reader.getCodedInputStream().get();

    for(;;)
    {
        const auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if (!ObfReaderUtilities::reachedDataEnd(cis))
                return;

            return;
        case OBF::OsmAndTransportIndex::kRoutesFieldNumber:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        case OBF::OsmAndTransportIndex::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, section->name);
            break;
        case OBF::OsmAndTransportIndex::kStopsFieldNumber:
            {
                section->_stopsLength = ObfReaderUtilities::readBigEndianInt(cis);
                section->_stopsOffset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(section->_stopsLength);

                readTransportStopsBounds(reader, section);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndTransportIndex::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto offset = cis->CurrentPosition();
                cis->Seek(offset + length);
            }
            //TODO:
            /*IndexStringTable st = new IndexStringTable();
            st.length = codedIS.readRawVarint32();
            st.fileOffset = codedIS.getTotalBytesRead();
            // Do not cache for now save memory
            // readStringTable(st, 0, 20, true);
            ind.stringTable = st;
            codedIS.seek(st.length + st.fileOffset);*/
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfTransportSectionReader_P::readTransportStopsBounds(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfTransportSectionInfo>& section)
{
    const auto cis = reader.getCodedInputStream().get();

    for(;;)
    {
        const auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if (!ObfReaderUtilities::reachedDataEnd(cis))
                return;

            return;
        case OBF::TransportStopsTree::kLeftFieldNumber:
            section->_area24.left() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kRightFieldNumber:
            section->_area24.right() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kTopFieldNumber:
            section->_area24.top() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kBottomFieldNumber:
            section->_area24.bottom() = ObfReaderUtilities::readSInt32(cis);
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfTransportSectionReader_P::searchTransportStops(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    QList< std::shared_ptr<const TransportStop> >* resultOut,
    const AreaI* const bbox31,
    ObfSectionInfo::StringTable* const stringTable,
    const TransportStopVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    const auto oldLimit = cis->PushLimit(section->length);
    
    searchTransportTreeBounds(
                              reader,
                              section,
                              resultOut,
                              bbox31,
                              stringTable,
                              visitor,
                              queryController);
    
    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfTransportSectionReader_P::searchTransportTreeBounds(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    QList< std::shared_ptr<const TransportStop> >* resultOut,
    const AreaI* const bbox31,
    ObfSectionInfo::StringTable* const stringTable,
    const TransportStopVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    int init = 0;
    int lastIndexResult = -1;
    AreaI cbbox31 = AreaI(0, 0, 0, 0);
    
    for (;;)
    {
        const auto tag = cis->ReadTag();
        if (init == 0xf)
        {
            // coordinates are init
            init = 0;
            if (cbbox31.right() < bbox31->left() || cbbox31.left() > bbox31->right() || cbbox31.top() > bbox31->bottom() || cbbox31.bottom() < bbox31->top())
                return;
        }
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;
                
                return;
            case OBF::TransportStopsTree::kBottomFieldNumber:
                cbbox31.bottom() = ObfReaderUtilities::readSInt32(cis) + bbox31->bottom();
                init |= 1;
                break;
            case OBF::TransportStopsTree::kLeftFieldNumber:
                cbbox31.left() = ObfReaderUtilities::readSInt32(cis) + bbox31->left();
                init |= 2;
                break;
            case OBF::TransportStopsTree::kRightFieldNumber:
                cbbox31.right() = ObfReaderUtilities::readSInt32(cis) + bbox31->right();
                init |= 4;
                break;
            case OBF::TransportStopsTree::kTopFieldNumber:
                cbbox31.top() = ObfReaderUtilities::readSInt32(cis) + bbox31->top();
                init |= 8;
                break;
            case OBF::TransportStopsTree::kLeafsFieldNumber:
            {
                int stopOffset = cis->TotalBytesRead();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                if (lastIndexResult == -1)
                    lastIndexResult = resultOut->size();
                
                std::shared_ptr<TransportStop> transportStop;
                readTransportStop(reader, section, stopOffset, transportStop, cbbox31, bbox31, stringTable, queryController);
                if (!visitor || visitor(transportStop))
                {
                    if (resultOut)
                        resultOut->push_back(transportStop);
                }
                
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::TransportStopsTree::kSubtreesFieldNumber:
            {
                // left, ... already initialized
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                int filePointer = cis->TotalBytesRead();
                const auto oldLimit = cis->PushLimit(length);
                searchTransportTreeBounds(
                                          reader,
                                          section,
                                          resultOut,
                                          &cbbox31,
                                          stringTable,
                                          visitor,
                                          queryController);
                cis->PopLimit(oldLimit);
                cis->Seek(filePointer + length);
                
                assert(lastIndexResult < 0);
                break;
            }
            case OBF::TransportStopsTree::kBaseIdFieldNumber:
            {
                gpb::uint64 baseId;
                cis->ReadVarint64(&baseId);
                if (lastIndexResult != -1)
                {
                    for (int i = lastIndexResult; i < resultOut->size(); i++)
                    {
                        auto rs = std::const_pointer_cast<TransportStop>(resultOut->at(i));
                        rs->id = ObfObjectId::fromRawId(rs->id.id + baseId);
                    }
                }
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfTransportSectionReader_P::readTransportStop(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    const uint32_t stopOffset,
    std::shared_ptr<TransportStop>& outTransportStop,
    const AreaI& cbbox31,
    const AreaI* const bbox31,
    ObfSectionInfo::StringTable* const stringTable,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    ObfObjectId id;
    PointI position31;
    ObfTransportSectionInfo::ReferencesToRoutes referencesToRoutes;

    int tag = gpb::internal::WireFormatLite::GetTagFieldNumber(cis->ReadTag());
    assert(OBF::TransportStop::kDxFieldNumber == tag);
    position31.x = ObfReaderUtilities::readSInt32(cis) + cbbox31.left();
    
    tag = gpb::internal::WireFormatLite::GetTagFieldNumber(cis->ReadTag());
    assert(OBF::TransportStop::kDyFieldNumber == tag);
    position31.y = ObfReaderUtilities::readSInt32(cis) + cbbox31.top();

    if (bbox31->right() < position31.x || bbox31->left() > position31.x || bbox31->top() > position31.y || bbox31->bottom() < position31.y)
    {
        cis->Skip(cis->BytesUntilLimit());
        return;
    }
    
    outTransportStop.reset(new TransportStop(section));
    outTransportStop->position31 = position31;
    outTransportStop->offset = stopOffset;
    for (;;)
    {
        const auto t = cis->ReadTag();
        tag = gpb::internal::WireFormatLite::GetTagFieldNumber(t);
        switch (tag)
        {
            case 0:
                outTransportStop->id = id;
                outTransportStop->referencesToRoutes = referencesToRoutes;
                
                return;
            case OBF::TransportStop::kRoutesFieldNumber:
                referencesToRoutes.push_back(stopOffset - ObfReaderUtilities::readSInt32(cis));
                break;
            case OBF::TransportStop::kNameEnFieldNumber:
            {
                if (stringTable)
                {
                    int i = ObfReaderUtilities::readSInt32(cis);
                    if (!stringTable->contains(i))
                        stringTable->insert(i, QString());
                    
                    outTransportStop->enName = i;
                }
                else
                {
                    ObfReaderUtilities::skipUnknownField(cis, tag);
                }
                break;
            }
            case OBF::TransportStop::kNameFieldNumber:
            {
                if (stringTable)
                {
                    int i = ObfReaderUtilities::readSInt32(cis);
                    if (!stringTable->contains(i))
                        stringTable->insert(i, QString());

                    outTransportStop->localizedName = i;
                }
                else
                {
                    ObfReaderUtilities::skipUnknownField(cis, tag);
                }
                break;
            }
            case OBF::TransportStop::kIdFieldNumber:
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&id));
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, t);
                break;
        }
    }
}

