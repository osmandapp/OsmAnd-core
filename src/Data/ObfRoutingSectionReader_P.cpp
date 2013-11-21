#include "ObfRoutingSectionReader_P.h"

#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"
#include "Road.h"
#include "IQueryFilter.h"
#include "ObfReaderUtilities.h"
#include "Utilities.h"

#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>

OsmAnd::ObfRoutingSectionReader_P::ObfRoutingSectionReader_P()
{
}

OsmAnd::ObfRoutingSectionReader_P::~ObfRoutingSectionReader_P()
{
}

void OsmAnd::ObfRoutingSectionReader_P::read( const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfRoutingSectionInfo>& section )
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
            ObfReaderUtilities::readQString(cis, section->_name);
            break;
        case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfRoutingSectionInfo_P::EncodingRule> encodingRule(new ObfRoutingSectionInfo_P::EncodingRule());
                encodingRule->_id = routeEncodingRuleId++;
                readEncodingRule(reader, section, encodingRule);

                cis->PopLimit(oldLimit);

                while((unsigned)section->_d->_encodingRules.size() < encodingRule->_id)
                    section->_d->_encodingRules.push_back(qMove(std::shared_ptr<ObfRoutingSectionInfo_P::EncodingRule>()));
                section->_d->_encodingRules.push_back(qMove(encodingRule));
            } 
            break;
        case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
        case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                const std::shared_ptr<ObfRoutingSubsectionInfo> subsection(new ObfRoutingSubsectionInfo(section));
                subsection->_length = ObfReaderUtilities::readBigEndianInt(cis);
                subsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(subsection->_length);

                readSubsectionHeader(reader, subsection, nullptr, 0);

                cis->PopLimit(oldLimit);

                if(tfn == OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber)
                    section->_subsections.push_back(qMove(subsection));
                else
                    section->_baseSubsections.push_back(qMove(subsection));
            }
            break;
        case OBF::OsmAndRoutingIndex::kBorderBoxFieldNumber:
        case OBF::OsmAndRoutingIndex::kBaseBorderBoxFieldNumber:
            {
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                if(tfn == OBF::OsmAndRoutingIndex::kBorderBoxFieldNumber)
                {
                    section->_d->_borderBoxLength = length;
                    section->_d->_borderBoxOffset = offset;
                }
                else
                {
                    section->_d->_baseBorderBoxLength = length;
                    section->_d->_baseBorderBoxOffset = offset;
                }
                cis->Skip(length);
            }
            break;
        case OBF::OsmAndRoutingIndex::kBlocksFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readEncodingRule(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    const std::shared_ptr<ObfRoutingSectionInfo_P::EncodingRule>& rule )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            {
                if(rule->_value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
                    rule->_value = QLatin1String("yes");
                if(rule->_value.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0)
                    rule->_value = QLatin1String("no");

                if(rule->_tag.compare(QLatin1String("oneway"), Qt::CaseInsensitive) == 0)
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::OneWay;
                    if(rule->_value == QLatin1String("-1") || rule->_value == QLatin1String("reverse"))
                        rule->_parsedValue.asSignedInt = -1;
                    else if(rule->_value == QLatin1String("1") || rule->_value == QLatin1String("yes"))
                        rule->_parsedValue.asSignedInt = 1;
                    else
                        rule->_parsedValue.asSignedInt = 0;
                }
                else if(rule->_tag.compare(QLatin1String("highway"), Qt::CaseInsensitive) == 0 && rule->_value == QLatin1String("traffic_signals"))
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::TrafficSignals;
                }
                else if(rule->_tag.compare(QLatin1String("railway"), Qt::CaseInsensitive) == 0 && (rule->_value == QLatin1String("crossing") || rule->_value == QLatin1String("level_crossing")))
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::RailwayCrossing;
                }
                else if(rule->_tag.compare(QLatin1String("roundabout"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::Roundabout;
                }
                else if(rule->_tag.compare(QLatin1String("junction"), Qt::CaseInsensitive) == 0 && rule->_value.compare(QLatin1String("roundabout"), Qt::CaseInsensitive) == 0)
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::Roundabout;
                }
                else if(rule->_tag.compare(QLatin1String("highway"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::Highway;
                }
                else if(rule->_tag.startsWith(QLatin1String("access")) && !rule->_value.isEmpty())
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::Access;
                }
                else if(rule->_tag.compare(QLatin1String("maxspeed"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::Maxspeed;
                    rule->_parsedValue.asFloat = Utilities::parseSpeed(rule->_value, -1.0);
                }
                else if (rule->_tag.compare(QLatin1String("lanes"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = ObfRoutingSectionInfo_P::EncodingRule::Lanes;
                    rule->_parsedValue.asSignedInt = Utilities::parseArbitraryInt(rule->_value, -1);
                }

            }
            return;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
            ObfReaderUtilities::readQString(cis, rule->_tag);
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
            ObfReaderUtilities::readQString(cis, rule->_value);
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber:
            {
                gpb::uint32 id;
                cis->ReadVarint32(&id);
                rule->_id = id;
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readSubsectionHeader(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfRoutingSubsectionInfo>& subsection,
    const std::shared_ptr<ObfRoutingSubsectionInfo>& parent, uint32_t depth /*= std::numeric_limits<uint32_t>::max()*/)
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
                auto dleft = ObfReaderUtilities::readSInt32(cis);
                subsection->_area31.left = dleft + (parent ? parent->_area31.left : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
            {
                auto dright = ObfReaderUtilities::readSInt32(cis);
                subsection->_area31.right = dright + (parent ? parent->_area31.right : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
            {
                auto dtop = ObfReaderUtilities::readSInt32(cis);
                subsection->_area31.top = dtop + (parent ? parent->_area31.top : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
            {
                auto dbottom = ObfReaderUtilities::readSInt32(cis);
                subsection->_area31.bottom = dbottom + (parent ? parent->_area31.bottom : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            {
                subsection->_dataOffset = ObfReaderUtilities::readBigEndianInt(cis);

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

                const std::shared_ptr<ObfRoutingSubsectionInfo> childSubsection(new ObfRoutingSubsectionInfo(subsection));
                childSubsection->_length = ObfReaderUtilities::readBigEndianInt(cis);
                childSubsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(childSubsection->_length);
                readSubsectionHeader(reader, childSubsection, subsection, depth - 1);
                
                cis->PopLimit(oldLimit);

                subsection->_subsections.push_back(qMove(childSubsection));
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readSubsectionChildrenHeaders(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfRoutingSubsectionInfo>& subsection,
    uint32_t depth /*= std::numeric_limits<uint32_t>::max()*/ )
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
                const std::shared_ptr<ObfRoutingSubsectionInfo> childSubsection(new ObfRoutingSubsectionInfo(subsection));
                childSubsection->_length = ObfReaderUtilities::readBigEndianInt(cis);
                childSubsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(childSubsection->_length);
                readSubsectionHeader(reader, childSubsection, subsection, depth - 1);
                
                cis->PopLimit(oldLimit);

                subsection->_subsections.push_back(qMove(childSubsection));
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::querySubsections(
    const std::unique_ptr<ObfReader_P>& reader, const QList< std::shared_ptr<ObfRoutingSubsectionInfo> >& in,
    QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor /*= nullptr*/)
{
    auto cis = reader->_codedInputStream.get();

    for(auto itSubsection = in.cbegin(); itSubsection != in.cend(); ++itSubsection)
    {
        const auto& subsection = *itSubsection;

        // If section is completely outside of bbox, skip it
        if(filter && !filter->acceptsArea(subsection->_area31))
            continue;
        
        // Load children if they are not yet loaded
        if(subsection->_subsectionsOffset != 0 && subsection->_subsections.isEmpty())
        {
            cis->Seek(subsection->_offset);
            auto oldLimit = cis->PushLimit(subsection->_length);
            cis->Skip(subsection->_subsectionsOffset - subsection->_offset);
            const auto contains = !filter || filter->acceptsArea(subsection->_area31);
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

void OsmAnd::ObfRoutingSectionReader_P::loadSubsectionData(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
    QList< std::shared_ptr<const Model::Road> >* resultOut /*= nullptr*/,
    QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const Model::Road>&)> visitor /*= nullptr*/ )
{
    auto cis = reader->_codedInputStream.get();

    cis->Seek(subsection->_offset + subsection->_dataOffset);
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);
    readSubsectionData(reader, subsection, resultOut, resultMapOut, filter, visitor);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfRoutingSectionReader_P::readSubsectionData(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
    QList< std::shared_ptr<const Model::Road> >* resultOut /*= nullptr*/,
    QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const Model::Road>&)> visitor /*= nullptr*/ )
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
                for(auto itEntry = resultsByInternalId.cbegin(); itEntry != resultsByInternalId.cend(); ++itEntry)
                {
                    const auto& road = itEntry.value();

                    // Fill names of roads from stringtable
                    for(auto itNameEntry = road->_names.begin(); itNameEntry != road->_names.end(); ++itNameEntry)
                    {
                        const auto encodedId = itNameEntry.value();
                        const uint32_t stringId = ObfReaderUtilities::decodeIntegerFromString(encodedId);

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
                readSubsectionRoadsIds(reader, subsection, roadsIdsTable);
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
                readRoad(reader, subsection, roadsIdsTable, internalId, road);
                resultsByInternalId.insert(internalId, road);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBlock::kRestrictionsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readSubsectionRestriction(reader, subsection, resultsByInternalId, roadsIdsTable);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBlock::kStringTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                ObfReaderUtilities::readStringTable(cis, roadNamesTable);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readSubsectionRoadsIds(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
    QList<uint64_t>& ids )
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
                id += ObfReaderUtilities::readSInt64(cis);
                ids.push_back(id);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readSubsectionRestriction(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
    const QMap< uint32_t, std::shared_ptr<Model::Road> >& roads, const QList<uint64_t>& roadsInternalIdToGlobalIdMap )
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
                originRoad->_restrictions.insert(destinationRoadId, static_cast<Model::RoadRestriction>(restrictionType));
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }       
}

void OsmAnd::ObfRoutingSectionReader_P::readRoad(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
    const QList<uint64_t>& idsTable, uint32_t& internalId, const std::shared_ptr<Model::Road>& road )
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
                    const uint32_t x = ObfReaderUtilities::readSInt32(cis) + dx;
                    const uint32_t y = ObfReaderUtilities::readSInt32(cis) + dy;

                    road->_points.push_back(qMove(PointI(
                        x << ShiftCoordinates,
                        y << ShiftCoordinates
                        )));

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

                    road->_names.insert(stringTag, ObfReaderUtilities::encodeIntegerToString(stringId));
                }
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::loadSubsectionBorderBoxLinesPoints(
    const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint /*= nullptr*/)
{
    if(section->_d->_borderBoxOffset == 0 || section->_d->_borderBoxLength == 0)
        return;

    auto cis = reader->_codedInputStream.get();
    cis->Seek(section->_d->_borderBoxOffset);
    auto oldLimit = cis->PushLimit(section->_d->_borderBoxLength);

    QList<uint32_t> pointsOffsets;
    readBorderBoxLinesHeaders(reader, nullptr, filter,
        [&] (const std::shared_ptr<const ObfRoutingBorderLineHeader>& borderLine)
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
    for(auto itOffset = pointsOffsets.cbegin(); itOffset != pointsOffsets.cend(); ++itOffset)
    {
        cis->Seek(*itOffset);
        gpb::uint32 length;
        cis->ReadVarint32(&length);
        auto oldLimit = cis->PushLimit(length);

        readBorderLinePoints(reader, resultOut, filter, visitorPoint);

        cis->PopLimit(oldLimit);
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readBorderBoxLinesHeaders(const std::unique_ptr<ObfReader_P>& reader,
    QList< std::shared_ptr<const ObfRoutingBorderLineHeader> >* resultOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitor /*= nullptr*/)
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

                const std::shared_ptr<ObfRoutingBorderLineHeader> line(new ObfRoutingBorderLineHeader());
                readBorderLineHeader(reader, line, offset);

                cis->PopLimit(oldLimit);

                bool isValid = true;
                if(filter)
                {
                    if(line->_x2present)
                        isValid = filter->acceptsArea(AreaI(line->_x, line->_y, line->_x2, line->_y));
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
                             resultOut->push_back(qMove(line));
                     }
                }
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderBox::kBlocksFieldNumber:
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readBorderLineHeader(
    const std::unique_ptr<ObfReader_P>& reader,
    const std::shared_ptr<ObfRoutingBorderLineHeader>& borderLine, uint32_t outerOffset)
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
            borderLine->_offset = outerOffset + ObfReaderUtilities::readBigEndianInt(cis);
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readBorderLinePoints(
    const std::unique_ptr<ObfReader_P>& reader,
    QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitor /*= nullptr*/
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
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&location.x));
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kYFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&location.y));
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kBaseIdFieldNumber:
            cis->ReadVarint64(&id);
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPointsBlock::kPointsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfRoutingBorderLinePoint> point(new ObfRoutingBorderLinePoint());
                readBorderLinePoint(reader, point);

                cis->PopLimit(oldLimit);

                point->_id += id;
                point->_location += location;
                id = point->_id;
                location = point->_location;

                bool valid = true;
                if(filter)
                    valid = filter->acceptsPoint(point->location);
                if(valid && visitor)
                    valid = visitor(point);
                if(valid && resultOut)
                    resultOut->push_back(qMove(point));
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfRoutingSectionReader_P::readBorderLinePoint(
    const std::unique_ptr<ObfReader_P>& reader,
    const std::shared_ptr<ObfRoutingBorderLinePoint>& point)
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
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&point->_location.x));
            break;
        case OBF::OsmAndRoutingIndex_RouteBorderPoint::kDyFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&point->_location.y));
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
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}
