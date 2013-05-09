#include "ObfRoutingSection.h"

#include "Logging.h"
#include <Utilities.h>
#include <ObfReader.h>
#include <Road.h>
#include <google/protobuf/wire_format_lite.h>

#include "OBF.pb.h"

OsmAnd::ObfRoutingSection::ObfRoutingSection( class ObfReader* owner )
    : ObfSection(owner)
    , _borderBoxOffset(0)
    , _baseBorderBoxOffset(0)
    , _borderBoxLength(0)
    , _baseBorderBoxLength(0)
{
}

OsmAnd::ObfRoutingSection::~ObfRoutingSection()
{
}

void OsmAnd::ObfRoutingSection::read( ObfReader* reader, std::shared_ptr<ObfRoutingSection> section )
{
    auto cis = reader->_codedInputStream.get();

    uint32_t routeEncodingRuleId = 1;
    for(;;)
    {
        auto tag = cis->ReadTag();
        auto tfn = gpb::internal::WireFormatLite::GetTagFieldNumber(tag);
        switch(tfn)
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex::kNameFieldNumber:
            ObfReader::readQString(cis, section->_name);
            break;
        case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<EncodingRule> encodingRule(new EncodingRule());
                encodingRule->_id = routeEncodingRuleId++;
                readEncodingRule(reader, section.get(), encodingRule.get());
                while((unsigned)section->_encodingRules.size() < encodingRule->_id)
                    section->_encodingRules.push_back(std::shared_ptr<EncodingRule>());
                section->_encodingRules.push_back(encodingRule);
                cis->PopLimit(oldLimit);
            } 
            break;
        case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
        case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                std::shared_ptr<Subsection> subsection(new Subsection(section));
                subsection->_length = ObfReader::readBigEndianInt(cis);
                subsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(subsection->_length);
                readSubsectionHeader(reader, subsection, nullptr, 0);
                if(tfn == OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber)
                    section->_subsections.push_back(subsection);
                else
                    section->_baseSubsections.push_back(subsection);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex::kBorderBoxFieldNumber:
        case OBF::OsmAndRoutingIndex::kBaseBorderBoxFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                if(tfn == OBF::OsmAndRoutingIndex::kBorderBoxFieldNumber)
                {
                    section->_borderBoxLength = length;
                    section->_borderBoxOffset = offset;
                }
                else
                {
                    section->_baseBorderBoxLength = length;
                    section->_baseBorderBoxOffset = offset;
                }
                cis->Skip(length);
            }
            break;
        case OBF::OsmAndRoutingIndex::kBlocksFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readEncodingRule( ObfReader* reader, ObfRoutingSection* section, EncodingRule* rule )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            {
                if(rule->_value.compare(QString("true"), Qt::CaseInsensitive) == 0)
                    rule->_value = "yes";
                if(rule->_value.compare(QString("false"), Qt::CaseInsensitive) == 0)
                    rule->_value = "no";

                if(rule->_tag.compare(QString("oneway"), Qt::CaseInsensitive) == 0)
                {
                    rule->_type = EncodingRule::OneWay;
                    if(rule->_value == "-1" || rule->_value == "reverse")
                        rule->_parsedValue.asSignedInt = -1;
                    else if(rule->_value == "1" || rule->_value == "yes")
                        rule->_parsedValue.asSignedInt = 1;
                    else
                        rule->_parsedValue.asSignedInt = 0;
                }
                else if(rule->_tag.compare(QString("highway"), Qt::CaseInsensitive) == 0 && rule->_value == "traffic_signals")
                {
                    rule->_type = EncodingRule::TrafficSignals;
                }
                else if(rule->_tag.compare(QString("railway"), Qt::CaseInsensitive) == 0 && (rule->_value == "crossing" || rule->_value == "level_crossing"))
                {
                    rule->_type = EncodingRule::RailwayCrossing;
                }
                else if(rule->_tag.compare(QString("roundabout"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = EncodingRule::Roundabout;
                }
                else if(rule->_tag.compare(QString("junction"), Qt::CaseInsensitive) == 0 && rule->_value.compare(QString("roundabout"), Qt::CaseInsensitive) == 0)
                {
                    rule->_type = EncodingRule::Roundabout;
                }
                else if(rule->_tag.compare(QString("highway"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = EncodingRule::Highway;
                }
                else if(rule->_tag.startsWith("access") && !rule->_value.isEmpty())
                {
                    rule->_type = EncodingRule::Access;
                }
                else if(rule->_tag.compare(QString("maxspeed"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = EncodingRule::Maxspeed;
                    rule->_parsedValue.asFloat = Utilities::parseSpeed(rule->_value, -1.0);
                }
                else if (rule->_tag.compare(QString("lanes"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = EncodingRule::Lanes;
                    rule->_parsedValue.asSignedInt = Utilities::parseArbitraryInt(rule->_value, -1);
                }

            }
            return;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
            ObfReader::readQString(cis, rule->_tag);
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
            ObfReader::readQString(cis, rule->_value);
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber:
            {
                gpb::uint32 id;
                cis->ReadVarint32(&id);
                rule->_id = id;
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readSubsectionHeader( ObfReader* reader, std::shared_ptr<Subsection> subsection, Subsection* parent, uint32_t depth/* = std::numeric_limits<uint32_t>::max()*/ )
{
    auto shouldReadSubsections = (depth > 0);

    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto lastPos = cis->CurrentPosition();
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber:
            {
                auto dleft = ObfReader::readSInt32(cis);
                subsection->_area31.left = dleft + (parent ? parent->_area31.left : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
            {
                auto dright = ObfReader::readSInt32(cis);
                subsection->_area31.right = dright + (parent ? parent->_area31.right : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
            {
                auto dtop = ObfReader::readSInt32(cis);
                subsection->_area31.top = dtop + (parent ? parent->_area31.top : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
            {
                auto dbottom = ObfReader::readSInt32(cis);
                subsection->_area31.bottom = dbottom + (parent ? parent->_area31.bottom : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            {
                subsection->_dataOffset = ObfReader::readBigEndianInt(cis);

                // In case we have data, we must read all subsections
                shouldReadSubsections = true;
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                if(subsection->_subsectionsOffset == 0)
                    subsection->_subsectionsOffset = lastPos;
                if(!shouldReadSubsections)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    break;
                }
                std::shared_ptr<Subsection> childSubsection(new Subsection(subsection));
                childSubsection->_length = ObfReader::readBigEndianInt(cis);
                childSubsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(childSubsection->_length);
                readSubsectionHeader(reader, childSubsection, subsection.get(), depth - 1);
                subsection->_subsections.push_back(childSubsection);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readSubsectionChildrenHeaders( ObfReader* reader, std::shared_ptr<Subsection> subsection, uint32_t depth /*= std::numeric_limits<uint32_t>::max()*/ )
{
    if(!subsection->_subsectionsOffset)
        return;

    const auto shouldReadSubsections = (depth > 0 || subsection->_dataOffset != 0);
    if(!shouldReadSubsections)
        return;

    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto lastPos = cis->CurrentPosition();
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                if(!shouldReadSubsections)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    break;
                }
                std::shared_ptr<Subsection> childSubsection(new Subsection(subsection));
                childSubsection->_length = ObfReader::readBigEndianInt(cis);
                childSubsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(childSubsection->_length);
                readSubsectionHeader(reader, childSubsection, subsection.get(), depth - 1);
                subsection->_subsections.push_back(childSubsection);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::querySubsections(
    ObfReader* reader,
    const QList< std::shared_ptr<Subsection> >& in,
    QList< std::shared_ptr<Subsection> >* resultOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    std::function<bool (std::shared_ptr<Subsection>)> visitor /*= nullptr*/ )
{
    auto cis = reader->_codedInputStream.get();

    for(auto itSubsection = in.begin(); itSubsection != in.end(); ++itSubsection)
    {
        auto subsection = *itSubsection;

        // If section is completely outside of bbox, skip it
        if(filter && filter->_bbox31 && !filter->_bbox31->intersects(subsection->_area31))
            continue;
        
        // Load children if they are not yet loaded
        if(subsection->_subsectionsOffset != 0 && subsection->_subsections.isEmpty())
        {
            cis->Seek(subsection->_offset);
            auto oldLimit = cis->PushLimit(subsection->_length);
            cis->Skip(subsection->_subsectionsOffset - subsection->_offset);
            const auto contains = !filter || !filter->_bbox31 || filter->_bbox31->intersects(subsection->_area31);
            readSubsectionChildrenHeaders(reader, subsection, contains ? std::numeric_limits<uint32_t>::max() : 1);
            cis->PopLimit(oldLimit);
        }

        querySubsections(reader, subsection->_subsections, resultOut, filter, visitor);

        if(!visitor || visitor(subsection))
        {
            if(resultOut)
                resultOut->push_back(subsection);
        }
    }
}

void OsmAnd::ObfRoutingSection::loadSubsectionData(
    ObfReader* reader, std::shared_ptr<Subsection> subsection,
    QList< std::shared_ptr<Model::Road> >* resultOut /*= nullptr*/,
    QMap< uint64_t, std::shared_ptr<Model::Road> >* resultMapOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    std::function<bool (std::shared_ptr<OsmAnd::Model::Road>)> visitor /*= nullptr*/ )
{
    auto cis = reader->_codedInputStream.get();

    cis->Seek(subsection->_offset + subsection->_dataOffset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readSubsectionData(reader, subsection, resultOut, resultMapOut, filter, visitor);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfRoutingSection::readSubsectionData(
    ObfReader* reader, std::shared_ptr<Subsection> subsection,
    QList< std::shared_ptr<Model::Road> >* resultOut /*= nullptr*/,
    QMap< uint64_t, std::shared_ptr<Model::Road> >* resultMapOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    std::function<bool (std::shared_ptr<OsmAnd::Model::Road>)> visitor /*= nullptr*/ )
{
    QStringList roadNamesTable;
    QList<uint64_t> roadsIdsTable;
    QMap< uint32_t, std::shared_ptr<Model::Road> > resultsByInternalId;

    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            {
                for(auto itEntry = resultsByInternalId.begin(); itEntry != resultsByInternalId.end(); ++itEntry)
                {
                    auto road = itEntry.value();

                    // Fix strings in road
                    for(auto itNameEntry = road->_names.begin(); itNameEntry != road->_names.end(); ++itNameEntry)
                    {
                        auto encodedId = itNameEntry.value();
                        uint32_t stringId = 0;
                        stringId |= (encodedId.at(1 + 0).unicode() & 0xff) << 8*0;
                        stringId |= (encodedId.at(1 + 1).unicode() & 0xff) << 8*1;
                        stringId |= (encodedId.at(1 + 2).unicode() & 0xff) << 8*2;
                        stringId |= (encodedId.at(1 + 3).unicode() & 0xff) << 8*3;

                        itNameEntry.value() = roadNamesTable[stringId];
                    }

                    if(!visitor || visitor(road))
                    {
                        if(resultOut)
                            resultOut->push_back(road);
                        if(resultMapOut)
                            resultMapOut->insert(road->_id, road);
                    }
                }
            }
            return;
        case OBF::OsmAndRoutingIndex_RouteDataBlock::kIdTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readSubsectionRoadsIds(reader, subsection.get(), roadsIdsTable);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBlock::kDataObjectsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<Model::Road> road(new Model::Road(subsection));
                uint32_t internalId;
                readRoad(reader, subsection.get(), roadsIdsTable, internalId, road.get());
                resultsByInternalId.insert(internalId, road);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBlock::kRestrictionsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readSubsectionRestriction(reader, subsection.get(), resultsByInternalId, roadsIdsTable);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                ObfReader::readStringTable(cis, roadNamesTable);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readSubsectionRoadsIds( ObfReader* reader, Subsection* subsection, QList<uint64_t>& ids )
{
    auto cis = reader->_codedInputStream.get();

    uint64_t id = 0;

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::IdTable::kRouteIdFieldNumber:
            {
                id += ObfReader::readSInt64(cis);
                ids.push_back(id);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readSubsectionRestriction( ObfReader* reader, Subsection* subsection, const QMap< uint32_t, std::shared_ptr<Model::Road> >& roads, const QList<uint64_t>& roadsInternalIdToGlobalIdMap )
{
    uint32_t originInternalId;
    uint32_t destinationInternalId;
    uint32_t restrictionType;

    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            {
                auto originRoad = roads[originInternalId];
                auto destinationRoadId = roadsInternalIdToGlobalIdMap[destinationInternalId];
                originRoad->_restrictions.insert(destinationRoadId, static_cast<Model::Road::Restriction>(restrictionType));
            }
            return;
        case OBF::RestrictionData::kFromFieldNumber:
            {
                cis->ReadVarint32(&originInternalId);
            }
            break;
        case OBF::RestrictionData::kToFieldNumber:
            {
                cis->ReadVarint32(&destinationInternalId);
            }
            break;
        case OBF::RestrictionData::kTypeFieldNumber:
            {
                cis->ReadVarint32(&restrictionType);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }       
}

void OsmAnd::ObfRoutingSection::readRoad( ObfReader* reader, Subsection* subsection, const QList<uint64_t>& idsTable, uint32_t& internalId, Model::Road* road )
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::RouteData::kPointsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                auto dx = subsection->_area31.left >> ShiftCoordinates;
                auto dy = subsection->_area31.top >> ShiftCoordinates;
                while(cis->BytesUntilLimit() > 0)
                {
                    uint32_t x = ObfReader::readSInt32(cis) + dx;
                    uint32_t y = ObfReader::readSInt32(cis) + dy;
                    road->_points.push_back(PointI(
                        x << ShiftCoordinates,
                        y << ShiftCoordinates
                        ));
                    dx = x;
                    dy = y;
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::RouteData::kPointTypesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 pointIdx;
                    cis->ReadVarint32(&pointIdx);

                    gpb::uint32 innerLength;
                    cis->ReadVarint32(&innerLength);
                    auto innerOldLimit = cis->PushLimit(innerLength);

                    auto& pointTypes = road->_pointsTypes.insert(pointIdx, QVector<uint32_t>()).value();
                    while(cis->BytesUntilLimit() > 0)
                    {
                        gpb::uint32 pointType;
                        cis->ReadVarint32(&pointType);
                        pointTypes.push_back(pointType);
                    }
                    cis->PopLimit(innerOldLimit);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::RouteData::kTypesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 type;
                    cis->ReadVarint32(&type);
                    road->_types.push_back(type);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::RouteData::kRouteIdFieldNumber:
            {
                gpb::uint32 id;
                cis->ReadVarint32(&id);
                internalId = id;
                if(id < idsTable.size())
                    road->_id = idsTable[id];
                else
                    road->_id = id;
            }
            break;
        case OBF::RouteData::kStringNamesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 stringTag;
                    cis->ReadVarint32(&stringTag);
                    gpb::uint32 stringId;
                    cis->ReadVarint32(&stringId);

                    char fakeString[] = {
                        '[',
                        static_cast<char>((stringId >> 8*0) & 0xff),
                        static_cast<char>((stringId >> 8*1) & 0xff),
                        static_cast<char>((stringId >> 8*2) & 0xff),
                        static_cast<char>((stringId >> 8*3) & 0xff),
                        ']'
                    };
                    auto fakeQString = QString::fromLocal8Bit(fakeString, sizeof(fakeString));
                    assert(fakeQString.length() == 6);
                    road->_names.insert(stringTag, fakeQString);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::loadSubsectionBorderBoxLinesPoints(
    ObfReader* reader,
    const ObfRoutingSection* section,
    QList< std::shared_ptr<BorderLinePoint> >* resultOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    std::function<bool (std::shared_ptr<BorderLineHeader>)> visitorLine /*= nullptr*/,
    std::function<bool (std::shared_ptr<BorderLinePoint>)> visitorPoint /*= nullptr*/)
{
    if(section->_borderBoxOffset == 0 || section->_borderBoxLength == 0)
        return;

    auto cis = reader->_codedInputStream.get();
    cis->Seek(section->_borderBoxOffset);
    auto oldLimit = cis->PushLimit(section->_borderBoxLength);

    QList<uint32_t> pointsOffsets;
    readBorderBoxLinesHeaders(reader, nullptr, filter,
        [&] (std::shared_ptr<OsmAnd::ObfRoutingSection::BorderLineHeader> borderLine)
        {
            auto valid = !visitorLine || visitorLine(borderLine);
            if(!valid)
                return false;

            pointsOffsets.push_back(borderLine->offset);

            return false;
        }
    );

    cis->PopLimit(oldLimit);

    qSort(pointsOffsets);
    for(auto itOffset = pointsOffsets.begin(); itOffset != pointsOffsets.end(); ++itOffset)
    {
        cis->Seek(*itOffset);
        gpb::uint32 length;
        cis->ReadVarint32(&length);
        auto oldLimit = cis->PushLimit(length);

        readBorderLinePoints(reader, resultOut, filter, visitorPoint);

        cis->PopLimit(oldLimit);
    }
}

void OsmAnd::ObfRoutingSection::readBorderBoxLinesHeaders(ObfReader* reader, 
    QList< std::shared_ptr<BorderLineHeader> >* resultOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    std::function<bool (std::shared_ptr<BorderLineHeader>)> visitor /*= nullptr*/)
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteBorderBox::kBorderLinesFieldNumber:
            {
                auto offset = cis->CurrentPosition();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<BorderLineHeader> line(new BorderLineHeader());
                readBorderLineHeader(reader, line.get(), offset);
                bool isValid = true;
                if(filter && filter->_bbox31)
                {
                    if(line->_x2present)
                        isValid = filter->_bbox31->intersects(line->_x, line->_y, line->_x2, line->_y);
                    else
                        isValid = false;
                    /*FIXME: borders approach
                    else if(ln.hasToy())
                        isValid = req.intersects(ln.getX(), ln.getY(), ln.getX(), ln.getToy());*/
                }
                if(isValid)
                {
                     if(!visitor || (visitor && visitor(line)))
                     {
                         if(resultOut)
                             resultOut->push_back(line);
                     }
                }
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderBox::kBlocksFieldNumber:
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readBorderLineHeader( ObfReader* reader, BorderLineHeader* borderLine, uint32_t outerOffset )
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteBorderLine::kXFieldNumber:
            cis->ReadVarint32(&borderLine->_x);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderLine::kYFieldNumber:
            cis->ReadVarint32(&borderLine->_y);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderLine::kToxFieldNumber:
            cis->ReadVarint32(&borderLine->_x2);
            borderLine->_x2present = true;
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderLine::kToyFieldNumber:
            cis->ReadVarint32(&borderLine->_y2);
            borderLine->_y2present = true;
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderLine::kShiftToPointsBlockFieldNumber:
            borderLine->_offset = outerOffset + ObfReader::readBigEndianInt(cis);
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readBorderLinePoints(
    ObfReader* reader,
    QList< std::shared_ptr<BorderLinePoint> >* resultOut /*= nullptr*/,
    QueryFilter* filter /*= nullptr*/,
    std::function<bool (std::shared_ptr<BorderLinePoint>)> visitor /*= nullptr*/
    )
{
    auto cis = reader->_codedInputStream.get();

    PointI location;
    gpb::uint64 id;

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kXFieldNumber:
            cis->ReadVarint32(&location.x);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kYFieldNumber:
            cis->ReadVarint32(&location.y);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kBaseIdFieldNumber:
            cis->ReadVarint64(&id);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kPointsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<BorderLinePoint> point(new BorderLinePoint());
                readBorderLinePoint(reader, point.get());
                point->_id += id;
                point->_location += location;
                bool valid = true;
                if(filter && filter->_bbox31)
                    valid = filter->_bbox31->contains(point->location);
                if(valid && visitor)
                    valid = visitor(point);
                if(valid && resultOut)
                    resultOut->push_back(point);
                id = point->_id;
                location = point->_location;
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSection::readBorderLinePoint( ObfReader* reader, BorderLinePoint* point )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteBorderPoint::kDxFieldNumber:
            cis->ReadVarint32(&point->_location.x);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPoint::kDyFieldNumber:
            cis->ReadVarint32(&point->_location.y);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPoint::kRoadIdFieldNumber:
            cis->ReadVarint64(&point->_id);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPoint::kDirectionFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                //TODO:p.direction = codedIS.readBool();
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPoint::kTypesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                while(cis->BytesUntilLimit() > 0)
                {
                    gpb::uint32 value;
                    cis->ReadVarint32(&value);
                    point->_types.push_back(value);
                }
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

OsmAnd::ObfRoutingSection::EncodingRule::EncodingRule()
{
}

OsmAnd::ObfRoutingSection::EncodingRule::~EncodingRule()
{
}

bool OsmAnd::ObfRoutingSection::EncodingRule::isRoundabout() const
{
    return _type == Roundabout;
}

int OsmAnd::ObfRoutingSection::EncodingRule::getDirection() const
{
    if(_type == OneWay)
        return _parsedValue.asSignedInt;

    return OsmAnd::Model::Road::Direction::TwoWay;
}

OsmAnd::ObfRoutingSection::Subsection::Subsection(std::shared_ptr<Subsection> parent)
    : _dataOffset(0)
    , _subsectionsOffset(0)
    , area31(_area31)
    , section(parent->section)
    , parent(parent)
{
}

OsmAnd::ObfRoutingSection::Subsection::Subsection(std::shared_ptr<ObfRoutingSection> section)
    : _dataOffset(0)
    , _subsectionsOffset(0)
    , area31(_area31)
    , section(section)
{
}

OsmAnd::ObfRoutingSection::Subsection::~Subsection()
{
}

bool OsmAnd::ObfRoutingSection::Subsection::containsData() const
{
    return _dataOffset != 0;
}

OsmAnd::ObfRoutingSection::BorderLineHeader::BorderLineHeader()
    : _x2present(false)
    , _y2present(false)
    , x(_x)
    , y(_y)
    , x2(_x2)
    , x2present(_x2present)
    , y2(_y2)
    , y2present(_y2present)
    , offset(_offset)
{
}

OsmAnd::ObfRoutingSection::BorderLineHeader::~BorderLineHeader()
{
}

OsmAnd::ObfRoutingSection::BorderLinePoint::BorderLinePoint()
    : id(_id)
    , location(_location)
    , types(_types)
    , distanceToStartPoint(_distanceToStartPoint)
    , distanceToEndPoint(_distanceToEndPoint)
{
}

OsmAnd::ObfRoutingSection::BorderLinePoint::~BorderLinePoint()
{
}

std::shared_ptr<OsmAnd::ObfRoutingSection::BorderLinePoint> OsmAnd::ObfRoutingSection::BorderLinePoint::bboxedClone( uint32_t x31 ) const
{
    std::shared_ptr<BorderLinePoint> clone(new BorderLinePoint());
    clone->_location = location;
    clone->_location.x = x31;
    clone->_id = std::numeric_limits<uint64_t>::max();
    return clone;
}
