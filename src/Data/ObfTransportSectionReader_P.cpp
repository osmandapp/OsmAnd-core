#include "ObfTransportSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfReader_P.h"
#include "ObfTransportSectionInfo.h"
#include "TransportStop.h"
#include "TransportStopExit.h"
#include "TransportRoute.h"
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
        case OBF::OsmAndTransportIndex::kIncompleteRoutesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                section->_incompleteRoutesLength = length;
                section->_incompleteRoutesOffset = cis->CurrentPosition();
                cis->Seek(section->incompleteRoutesLength + section->incompleteRoutesOffset);
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
    const auto cis = reader.getCodedInputStream().get();
    auto values = stringTable->keys();
    qSort(values);
    cis->Seek(section->stringTable->fileOffset);
    const auto oldLimit = cis->PushLimit(section->stringTable->length);
    int current = 0;
    int i = 0;
    while (i < values.size() && cis->BytesUntilLimit() > 0)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                break;
            case OBF::StringTable::kSFieldNumber:
                if (current == values[i])
                {
                    QString value;
                    ObfReaderUtilities::readQString(cis, value);
                    stringTable->insert(values[i], value);
                    i++;
                }
                else
                {
                    ObfReaderUtilities::skipUnknownField(cis, tag);
                }
                current++;
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfTransportSectionReader_P::initializeNames(
    bool onlyDescription,
    ObfSectionInfo::StringTable* const stringTable,
    std::shared_ptr<TransportRoute> r)
{
    if (!r->localizedName.isEmpty())
        r->localizedName = stringTable->value(r->localizedName[0].unicode());
    
    if (!r->enName.isEmpty())
        r->enName = stringTable->value(r->enName[0].unicode());

    if (!r->oper.isEmpty())
        r->oper = stringTable->value(r->oper[0].unicode());

    if (!r->color.isEmpty())
        r->color = stringTable->value(r->color[0].unicode());

    if (!r->type.isEmpty())
        r->type = stringTable->value(r->type[0].unicode());

    if (!onlyDescription)
    {
        for (auto s : r->forwardStops)
            initializeNames(stringTable, s);
    }
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

QString OsmAnd::ObfTransportSectionReader_P::regStr(
    const ObfReader_P& reader,
    ObfSectionInfo::StringTable* const stringTable)
{
    if (stringTable)
    {
        const auto cis = reader.getCodedInputStream().get();
        auto i = ObfReaderUtilities::readLength(cis);
        if (!stringTable->contains(i))
            stringTable->insert(i, QString());
        return QChar(i);
    }
    return QString();
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
    outTransportStop->location = LatLon(Utilities::getLatitudeFromTile(TRANSPORT_STOP_ZOOM, position31.y), Utilities::getLongitudeFromTile(TRANSPORT_STOP_ZOOM, position31.x));
    outTransportStop->offset = stopOffset;
    for (;;)
    {
        const auto t = cis->ReadTag();
        tag = gpb::internal::WireFormatLite::GetTagFieldNumber(t);
        switch (tag)
        {
            case 0:
            {
                outTransportStop->id = id;
                outTransportStop->referencesToRoutes = referencesToRoutes;
                
                return outTransportStop;
            }
            case OBF::TransportStop::kRoutesFieldNumber:
            {
                referencesToRoutes.push_back(stopOffset - ObfReaderUtilities::readLength(cis));
                break;
            }
            case OBF::TransportStop::kNameEnFieldNumber:
            {
                if (stringTable)
                    outTransportStop->enName = regStr(reader, stringTable);
                else
                    ObfReaderUtilities::skipUnknownField(cis, tag);

                break;
            }
            case OBF::TransportStop::kNameFieldNumber:
            {
                if (stringTable)
                    outTransportStop->localizedName = regStr(reader, stringTable);
                else
                    ObfReaderUtilities::skipUnknownField(cis, tag);

                break;
            }
            case OBF::TransportStop::kExitsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);
                
                auto transportStopExit = readTransportStopExit(reader, section, cbbox31, stringTable);
                outTransportStop->addExit(transportStopExit);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::TransportStop::kIdFieldNumber:
            {
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&id));
                break;
            }
            default:
            {
                ObfReaderUtilities::skipUnknownField(cis, t);
                break;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<OsmAnd::TransportStopExit> OsmAnd::ObfTransportSectionReader_P::readTransportStopExit(const ObfReader_P& reader,
                                                         const std::shared_ptr<const ObfTransportSectionInfo>& section,
                                                         const AreaI& cbbox31,
                                                         ObfSectionInfo::StringTable* const stringTable)
{
    const auto cis = reader.getCodedInputStream().get();
    auto dataObject = std::make_shared<TransportStopExit>();
    int32_t x = 0;
    int32_t y = 0;
    
    while (true) {
        const auto t = cis->ReadTag();
        int tag = gpb::internal::WireFormatLite::GetTagFieldNumber(t);
        
        switch (tag) {
            case 0:
                if (dataObject->enName.length() == 0)
                {
                    if (stringTable)
                        dataObject->enName = regStr(reader, stringTable);
                    else
                        ObfReaderUtilities::skipUnknownField(cis, tag);
                }
                if (x != 0 || y != 0)
                {
                    dataObject->setLocation(TRANSPORT_STOP_ZOOM, x, y);
                }
                return dataObject;
            case OBF::TransportStopExit::kRefFieldNumber:
                if (stringTable)
                    dataObject->ref = regStr(reader, stringTable);
                else
                    ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
            case OBF::TransportStopExit::kDxFieldNumber:
                x = ObfReaderUtilities::readSInt32(cis) + cbbox31.left();
                break;
            case OBF::TransportStopExit::kDyFieldNumber:
                y = ObfReaderUtilities::readSInt32(cis) + cbbox31.top();
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

std::shared_ptr<OsmAnd::TransportRoute> OsmAnd::ObfTransportSectionReader_P::getTransportRoute(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    const uint32_t routeOffset,
    ObfSectionInfo::StringTable* const stringTable,
    bool onlyDescription)
{
    const auto cis = reader.getCodedInputStream().get();
    
    cis->Seek(routeOffset);
    
    gpb::uint32 routeLength;
    cis->ReadVarint32(&routeLength);
    const auto oldLimit = cis->PushLimit(routeLength);
    
    auto dataObject = std::make_shared<TransportRoute>();
    bool end = false;
    ObfObjectId rid = ObfObjectId::invalidId();
    int32_t rx = 0;
    int32_t ry = 0;
    
    dataObject->offset = routeOffset;
    while (!end)
    {
        const auto t = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(t))
        {
            case 0:
                end = true;
                break;
            case OBF::TransportRoute::kDistanceFieldNumber:
                dataObject->dist = ObfReaderUtilities::readLength(cis);
                break;
            case OBF::TransportRoute::kIdFieldNumber:
            {
                ObfObjectId id;
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&id));
                dataObject->id = id;
                break;
            }
            case OBF::TransportRoute::kRefFieldNumber:
                ObfReaderUtilities::readQString(cis, dataObject->ref);
                break;
            case OBF::TransportRoute::kTypeFieldNumber:
                if (stringTable)
                    dataObject->type = regStr(reader, stringTable);
                break;
            case OBF::TransportRoute::kNameEnFieldNumber:
                if (stringTable)
                    dataObject->enName = regStr(reader, stringTable);
                break;
            case OBF::TransportRoute::kNameFieldNumber:
                if (stringTable)
                    dataObject->localizedName = regStr(reader, stringTable);
                break;
            case OBF::TransportRoute::kOperatorFieldNumber:
                if (stringTable)
                    dataObject->oper = regStr(reader, stringTable);
                break;
            case OBF::TransportRoute::kColorFieldNumber:
                if (stringTable)
                    dataObject->color = regStr(reader, stringTable);
                break;
            case OBF::TransportRoute::kGeometryFieldNumber:
            {
                gpb::uint32 sizeL;
                cis->ReadVarint32(&sizeL);
                const auto pold = cis->PushLimit(sizeL);
                int32_t px = 0;
                int32_t py = 0;
                QVector<PointI> w;
                while (cis->BytesUntilLimit() > 0)
                {
                    auto ddx = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);
                    auto ddy = (ObfReaderUtilities::readSInt32(cis) << ShiftCoordinates);

                    if (ddx == 0 && ddy == 0)
                    {
                        if (w.size() > 0)
                            dataObject->forwardWays31.push_back(qMove(w));
                        
                        w.clear();
                    }
                    else
                    {
                        auto x = ddx + px;
                        auto y = ddy + py;
                        w.push_back(PointI(x, y));
                        px = x;
                        py = y;
                    }
                }
                if (w.size() > 0)
                    dataObject->forwardWays31.push_back(qMove(w));
                
                cis->PopLimit(pold);
                break;
            }
            case OBF::TransportRoute::kDirectStopsFieldNumber:
            {
                if (onlyDescription)
                {
                    end = true;
                    cis->Skip(cis->BytesUntilLimit());
                    break;
                }
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto olds = cis->PushLimit(length);
                auto stop = readTransportRouteStop(reader, section, routeOffset, rx, ry, rid, stringTable);
                dataObject->forwardStops.push_back(stop);
                rid = stop->id;
                rx = Utilities::getTileNumberX(TRANSPORT_STOP_ZOOM, stop->location.longitude);
                ry = Utilities::getTileNumberY(TRANSPORT_STOP_ZOOM, stop->location.latitude);
                cis->PopLimit(olds);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, t);
                break;
        }
    }

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);

    return dataObject;
}

std::shared_ptr<OsmAnd::TransportStop> OsmAnd::ObfTransportSectionReader_P::readTransportRouteStop(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    const uint32_t routeOffset,
    int32_t dx,
    int32_t dy,
    ObfObjectId did,
    ObfSectionInfo::StringTable* const stringTable)
{
    const auto cis = reader.getCodedInputStream().get();
    
    auto dataObject = std::make_shared<TransportStop>(section);
    dataObject->offset = cis->TotalBytesRead();
    // dataObject.setReferencesToRoutes(new int[] {filePointer});
    bool end = false;
    while (!end)
    {
        const auto t = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(t))
        {
            case 0:
                end = true;
                break;
            case OBF::TransportRouteStop::kNameEnFieldNumber:
                dataObject->enName = regStr(reader, stringTable);
                break;
            case OBF::TransportRouteStop::kNameFieldNumber:
                dataObject->localizedName = regStr(reader, stringTable);
                break;
            case OBF::TransportRouteStop::kIdFieldNumber:
                did.id += ObfReaderUtilities::readSInt64(cis);
                break;
            case OBF::TransportRouteStop::kDxFieldNumber:
                dx += ObfReaderUtilities::readSInt32(cis);
                break;
            case OBF::TransportRouteStop::kDyFieldNumber:
                dy += ObfReaderUtilities::readSInt32(cis);
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, t);
                break;
        }
    }
    dataObject->id = did;
    dataObject->location = LatLon(Utilities::getLatitudeFromTile(TRANSPORT_STOP_ZOOM, dy), Utilities::getLongitudeFromTile(TRANSPORT_STOP_ZOOM, dx));
    return dataObject;
}

