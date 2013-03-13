#include "ObfRoutingSection.h"

#include <ObfReader.h>
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

void OsmAnd::ObfRoutingSection::read( ObfReader* reader, ObfRoutingSection* section )
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
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                section->_name = QString::fromStdString(name);
            }
            break;
        case OBF::OsmAndRoutingIndex::kRulesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<EncodingRule> encodingRule(new EncodingRule());
                encodingRule->_id = routeEncodingRuleId++;
                readEncodingRule(reader, section, encodingRule.get());
                while((unsigned)section->_encodingRules.size() <= encodingRule->_id)
                    section->_encodingRules.push_back(std::shared_ptr<EncodingRule>());
                section->_encodingRules.push_back(encodingRule);
                cis->PopLimit(oldLimit);
            } 
            break;
        case OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
        case OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber:
            {
                std::shared_ptr<Subsection> subsection(new Subsection());
                subsection->_length = ObfReader::readBigEndianInt(cis);
                subsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(subsection->_length);
                readSubsection(reader, section, subsection.get(), nullptr);
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
                    rule->_parsedValue.asFloat = -1.0f;
                    if(rule->_value == "none")
                    {
                        //TODO:rule->_parsedValue.asFloat = RouteDataObject.NONE_MAX_SPEED;
                    }
                    else
                    {
                        /*TODO:
                        int i = 0;
                        while (i < v.length() && Character.isDigit(v.charAt(i))) {
                            i++;
                        }
                        if (i > 0) {
                            floatValue = Integer.parseInt(v.substring(0, i));
                            floatValue /= 3.6; // km/h -> m/s
                            if (v.contains("mph")) {
                                floatValue *= 1.6;
                            }
                        }
                        */
                    }
                }
                else if (rule->_tag.compare(QString("lanes"), Qt::CaseInsensitive) == 0 && !rule->_value.isEmpty())
                {
                    rule->_type = EncodingRule::Lanes;
                    rule->_parsedValue.asSignedInt = -1;
                    /*TODO:
                    int i = 0;
                    while (i < v.length() && Character.isDigit(v.charAt(i))) {
                        i++;
                    }
                    if (i > 0) {
                        intValue = Integer.parseInt(v.substring(0, i));
                    }
                    */
                }

            }
            return;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
            {
                std::string tag_;
                gpb::internal::WireFormatLite::ReadString(cis, &tag_);
                rule->_tag = QString::fromStdString(tag_);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
            {
                std::string value;
                gpb::internal::WireFormatLite::ReadString(cis, &value);
                rule->_value = QString::fromStdString(value);
            }
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

void OsmAnd::ObfRoutingSection::readSubsection( ObfReader* reader, ObfRoutingSection* section, Subsection* subsection, Subsection* parent )
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber:
            {
                auto dleft = ObfReader::readSInt32(cis);
                subsection->_left31 = dleft + (parent ? parent->_left31 : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber:
            {
                auto dright = ObfReader::readSInt32(cis);
                subsection->_right31 = dright + (parent ? parent->_right31 : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber:
            {
                auto dtop = ObfReader::readSInt32(cis);
                subsection->_top31 = dtop + (parent ? parent->_top31 : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber:
            {
                auto dbottom = ObfReader::readSInt32(cis);
                subsection->_bottom31 = dbottom + (parent ? parent->_bottom31 : 0);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber:
            {
                subsection->_dataOffset = ObfReader::readBigEndianInt(cis);
            }
            break;
        case OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber:
            {
                std::shared_ptr<Subsection> childSubsection(new Subsection());
                childSubsection->_length = ObfReader::readBigEndianInt(cis);
                childSubsection->_offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(childSubsection->_length);
                readSubsection(reader, section, childSubsection.get(), subsection);
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

OsmAnd::ObfRoutingSection::EncodingRule::EncodingRule()
{
}

OsmAnd::ObfRoutingSection::EncodingRule::~EncodingRule()
{
}

OsmAnd::ObfRoutingSection::Subsection::Subsection()
    : _dataOffset(0)
{
}

OsmAnd::ObfRoutingSection::Subsection::~Subsection()
{
}
