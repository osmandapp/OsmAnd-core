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
                ObfTransportSectionInfo::IndexStringTable st;
                st.fileOffset = offset;
                st.length = length;
                section->_stringTable = st;
                cis->Seek(offset + length);
            }
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
            section->_area31.left() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kRightFieldNumber:
            section->_area31.right() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kTopFieldNumber:
            section->_area31.top() = ObfReaderUtilities::readSInt32(cis);
            break;
        case OBF::TransportStopsTree::kBottomFieldNumber:
            section->_area31.bottom() = ObfReaderUtilities::readSInt32(cis);
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfTransportSectionReader_P::initializeStringTable(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    ObfSectionInfo::StringTable* const stringTable)
{
    
}

void OsmAnd::ObfTransportSectionReader_P::initializeNames(
    ObfSectionInfo::StringTable* const stringTable,
    std::shared_ptr<TransportStop> s)
{
    if (!s->localizedName.isEmpty())
        s->localizedName = stringTable->value(s->localizedName[0].unicode());
    
    if (!s->enName.isEmpty())
        s->enName = stringTable->value(s->enName[0].unicode());
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
    cis->Seek(section->stopsOffset);
    const auto oldLimit = cis->PushLimit(section->stopsLength);
    QList< std::shared_ptr<TransportStop> > res;
    auto pbbox31 = AreaI(0, 0, 0, 0);
    
    searchTransportTreeBounds(
                              reader,
                              section,
                              res,
                              pbbox31,
                              bbox31,
                              stringTable,
                              queryController);
    
    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
    
    if (stringTable)
    {
        initializeStringTable(reader, section, stringTable);
        for (auto st : res)
            initializeNames(stringTable, st);
    }
    for (auto transportStop : res)
    {
        if (!visitor || visitor(transportStop))
        {
            if (resultOut)
                resultOut->push_back(transportStop);
        }
    }
}

void OsmAnd::ObfTransportSectionReader_P::searchTransportTreeBounds(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    QList< std::shared_ptr<TransportStop> >& resultOut,
    AreaI& pbbox31,
    const AreaI* const bbox31,
    ObfSectionInfo::StringTable* const stringTable,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    int init = 0;
    QList< std::shared_ptr<TransportStop> > res;
    auto cbbox31 = AreaI(0, 0, 0, 0);
    
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

                for (auto ts : res)
                    resultOut.append(res);
                
                return;
            case OBF::TransportStopsTree::kBottomFieldNumber:
                cbbox31.bottom() = ObfReaderUtilities::readSInt32(cis) + pbbox31.bottom();
                init |= 1;
                break;
            case OBF::TransportStopsTree::kLeftFieldNumber:
                cbbox31.left() = ObfReaderUtilities::readSInt32(cis) + pbbox31.left();
                init |= 2;
                break;
            case OBF::TransportStopsTree::kRightFieldNumber:
                cbbox31.right() = ObfReaderUtilities::readSInt32(cis) + pbbox31.right();
                init |= 4;
                break;
            case OBF::TransportStopsTree::kTopFieldNumber:
                cbbox31.top() = ObfReaderUtilities::readSInt32(cis) + pbbox31.top();
                init |= 8;
                break;
            case OBF::TransportStopsTree::kLeafsFieldNumber:
            {
                int stopOffset = cis->TotalBytesRead();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);
                
                auto transportStop = readTransportStop(reader, section, stopOffset, cbbox31, bbox31, stringTable, queryController);
                if (transportStop)
                    res.push_back(transportStop);
                
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::TransportStopsTree::kSubtreesFieldNumber:
            {
                // left, ... already initialized
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                int filePointer = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);
                searchTransportTreeBounds(
                                          reader,
                                          section,
                                          res,
                                          cbbox31,
                                          bbox31,
                                          stringTable,
                                          queryController);
                cis->PopLimit(oldLimit);
                cis->Seek(filePointer + length);
                
                break;
            }
            case OBF::TransportStopsTree::kBaseIdFieldNumber:
            {
                gpb::uint64 baseId;
                cis->ReadVarint64(&baseId);
                for (auto transportStop : res)
                    transportStop->id = ObfObjectId::fromRawId(transportStop->id.id + baseId);

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

std::shared_ptr<OsmAnd::TransportStop> OsmAnd::ObfTransportSectionReader_P::readTransportStop(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    const uint32_t stopOffset,
    const AreaI& cbbox31,
    const AreaI* const bbox31,
    ObfSectionInfo::StringTable* const stringTable,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    ObfObjectId id;
    PointI position31;
    QVector<uint32_t> referencesToRoutes;

    int tag = gpb::internal::WireFormatLite::GetTagFieldNumber(cis->ReadTag());
    assert(OBF::TransportStop::kDxFieldNumber == tag);
    position31.x = ObfReaderUtilities::readSInt32(cis) + cbbox31.left();
    
    tag = gpb::internal::WireFormatLite::GetTagFieldNumber(cis->ReadTag());
    assert(OBF::TransportStop::kDyFieldNumber == tag);
    position31.y = ObfReaderUtilities::readSInt32(cis) + cbbox31.top();

    if (bbox31->right() < position31.x || bbox31->left() > position31.x || bbox31->top() > position31.y || bbox31->bottom() < position31.y)
    {
        cis->Skip(cis->BytesUntilLimit());
        return nullptr;
    }
    auto outTransportStop = std::make_shared<TransportStop>(section);
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
                
                return outTransportStop;
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
                    
                    outTransportStop->enName = QChar(i);
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

                    outTransportStop->localizedName = QChar(i);
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
    return nullptr;
}

