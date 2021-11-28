#include "ObfAddressSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "QtCommon.h"

#include "OsmAndCore.h"
#include "Common.h"
#include "Nullable.h"
#include "ObfReader.h"
#include "ObfReader_P.h"
#include "ObfAddressSectionInfo.h"
#include "StreetGroup.h"
#include "Street.h"
#include "Building.h"
#include "StreetIntersection.h"
#include "ObfReaderUtilities.h"
#include "IQueryController.h"
#include "Utilities.h"
#include "DataCommonTypes.h"
#include "Logging.h"

OsmAnd::ObfAddressSectionReader_P::ObfAddressSectionReader_P()
{
}

OsmAnd::ObfAddressSectionReader_P::~ObfAddressSectionReader_P()
{
}

void OsmAnd::ObfAddressSectionReader_P::read(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfAddressSectionInfo>& section)
{
    const auto cis = reader.getCodedInputStream().get();

    Nullable<AreaI> area31;

    for (;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (area31.isSet())
                    section->area31 = *area31;
                else
                    section->area31 = AreaI::largestPositive();

                return;
            case OBF::OsmAndAddressIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, section->name);
                break;
            case OBF::OsmAndAddressIndex::kNameEnFieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);

                section->localizedNames.insert(QLatin1String("en"), value);
                break;
            }
            case OBF::OsmAndAddressIndex::kBoundariesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                AreaI bbox31;
                ObfReaderUtilities::readTileBox(cis, bbox31);
                area31 = bbox31;

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndAddressIndex::kAttributeTagsTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                ObfReaderUtilities::readStringTable(cis, section->attributeTagsTable);
                
                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndAddressIndex::kCitiesFieldNumber:
            {
                auto name = QString();
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto offset = cis->CurrentPosition();
                uint32_t type = 1;
                while (true)
                {
                    const auto tt = cis->ReadTag();
                    const auto ttag = gpb::internal::WireFormatLite::GetTagFieldNumber(tt);
                    if (ttag == 0)
                    {
                        break;
                    }
                    else if (ttag == OBF::OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber)
                    {
                        type = ObfReaderUtilities::readLength(cis);
                        break;
                    }
                    else
                    {
                        ObfReaderUtilities::skipUnknownField(cis, tt);
                    }
                }
                auto block = std::make_shared<ObfAddressSectionInfo::CitiesBlock>(name, offset, length, type);
                section->cities.push_back(block);
                
                cis->Seek(block->offset + block->length);
                break;
            }
            case OBF::OsmAndAddressIndex::kNameIndexFieldNumber:
                section->nameIndexInnerOffset = tagPos - section->offset;
                ObfReaderUtilities::skipUnknownField(cis, tag);
                /*
                 auto offset = cis->CurrentPosition();
                 section->nameIndexInnerOffset = offset - section->offset;
                 auto length = ObfReaderUtilities::readBigEndianInt(cis);
                 cis->Seek(offset + length + 4);
                 */
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetGroups(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const StreetGroup> >* resultOut,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const StreetGroupVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    ObfAddressStreetGroupType type = ObfAddressStreetGroupType::Unknown;
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndAddressIndex_CitiesIndex::kCitiesFieldNumber:
            {
                const auto offset = cis->CurrentPosition();
                const auto length = ObfReaderUtilities::readLength(cis);
                const auto oldLimit = cis->PushLimit(length);
                
                std::shared_ptr<OsmAnd::StreetGroup> streetGroup;
                readStreetGroup(reader, section, type, offset, streetGroup, bbox31, queryController);
                ObfReaderUtilities::ensureAllDataWasRead(cis);
                
                cis->PopLimit(oldLimit);
                
                if (streetGroup)
                {
                    if (!visitor || visitor(streetGroup))
                    {
                        if (resultOut)
                            resultOut->push_back(streetGroup);
                    }
                }
                
                if (queryController && queryController->isAborted())
                    ObfReaderUtilities::skipBlockWithLength(cis);
                
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetGroup(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    const ObfAddressStreetGroupType streetGroupType,
    const uint32_t groupOffset,
    std::shared_ptr<StreetGroup>& outStreetGroup,
    const AreaI* const bbox31,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    auto subtype = ObfAddressStreetGroupSubtype::Unknown;
    QString nativeName;
    QHash<QString, QString> localizedNames;
    ObfObjectId id;
    bool autogenerateId = true;
    PointI position31;
    uint32_t dataOffset = 0;
    
    QStringList additionalTags;
    QStringList attributeTagsTable;
    if (section)
        attributeTagsTable = section->attributeTagsTable;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (bbox31 && !bbox31->contains(position31))
                    return;

                outStreetGroup.reset(new StreetGroup(section));
                if (autogenerateId)
                    outStreetGroup->id = ObfObjectId::generateUniqueId(groupOffset, section);
                else
                    outStreetGroup->id = id;
                
                if(streetGroupType == ObfAddressStreetGroupType::Unknown) {
                    // make assumption based on the data
                    if(subtype == ObfAddressStreetGroupSubtype::City || subtype == ObfAddressStreetGroupSubtype::Town) {
                        outStreetGroup->type = ObfAddressStreetGroupType::CityOrTown;
                    } else if(subtype == ObfAddressStreetGroupSubtype::Unknown) {
                        outStreetGroup->type = ObfAddressStreetGroupType::Postcode;
                    } else {
                        outStreetGroup->type = ObfAddressStreetGroupType::Village;
                    }
                } else {
                    outStreetGroup->type = streetGroupType;
                }
                outStreetGroup->subtype = subtype;
                outStreetGroup->nativeName = nativeName;
                outStreetGroup->localizedNames = localizedNames;
                outStreetGroup->position31 = position31;
                outStreetGroup->dataOffset = dataOffset;

                return;
            case OBF::CityIndex::kCityTypeFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&subtype));
                break;
            case OBF::CityIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, nativeName);
                break;
            case OBF::CityIndex::kNameEnFieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);
                localizedNames.insert(QLatin1String("en"), value);
                break;
            }
            case OBF::CityIndex::kAttributeTagIdsFieldNumber:
            {
                const auto tgid = ObfReaderUtilities::readLength(cis);
                if (tgid < attributeTagsTable.count())
                    additionalTags << attributeTagsTable[tgid];
                break;
            }
            case OBF::CityIndex::kAttributeValuesFieldNumber:
            {
                QString nm;
                ObfReaderUtilities::readQString(cis, nm);
                if (!additionalTags.isEmpty())
                {
                    QString tg = additionalTags.front();
                    additionalTags.pop_front();
                    if (tg.startsWith(QLatin1String("name:")))
                        localizedNames.insert(tg.mid(5), nm);
                }
                break;
            }
            case OBF::CityIndex::kIdFieldNumber:
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&id));
                autogenerateId = false;
                break;
            case OBF::CityIndex::kXFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&position31.x));
                break;
            case OBF::CityIndex::kYFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&position31.y));
                break;
            case OBF::CityIndex::kShiftToCityBlockIndexFieldNumber:
            {
                const auto offset = ObfReaderUtilities::readBigEndianInt(cis);
                dataOffset = groupOffset + offset;
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetsFromGroup(
    const ObfReader_P& reader,
    const std::shared_ptr<const StreetGroup>& streetGroup,
    QList< std::shared_ptr<const Street> >* resultOut,
    const AreaI* const bbox31,
    const StreetVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::CityBlockIndex::kStreetsFieldNumber:
            {
                gpb::uint32 length;
                const auto offset = cis->CurrentPosition();
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<Street> street;
                readStreet(reader, streetGroup, offset, street, bbox31, queryController);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                if (!visitor || visitor(street))
                {
                    if (resultOut)
                        resultOut->push_back(street);
                }
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreet(
    const ObfReader_P& reader,
    const std::shared_ptr<const StreetGroup>& streetGroup,
    const uint32_t streetOffset,
    std::shared_ptr<Street>& outStreet,
    const AreaI* const bbox31,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    QString nativeName;
    QHash<QString, QString> localizedNames;
    PointI position31;
    ObfObjectId id;
    bool autogenerateId = true;
    uint32_t firstBuildingInnerOffset = 0;
    uint32_t firstIntersectionInnerOffset = 0;
    
    QStringList additionalTags;
    QStringList attributeTagsTable;
    if (streetGroup && streetGroup->obfSection)
        attributeTagsTable = streetGroup->obfSection->attributeTagsTable;

    for (;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (bbox31 && !bbox31->contains(position31))
                    return;

                outStreet.reset(new Street(streetGroup));
                if (autogenerateId)
                    outStreet->id = ObfObjectId::generateUniqueId(streetOffset, streetGroup->obfSection);
                else
                    outStreet->id = id;
                outStreet->nativeName = nativeName;
                outStreet->localizedNames = localizedNames;
                outStreet->position31 = position31;
                outStreet->offset = streetOffset;
                outStreet->firstBuildingInnerOffset = firstBuildingInnerOffset;
                outStreet->firstIntersectionInnerOffset = firstIntersectionInnerOffset;

                return;
            case OBF::StreetIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, nativeName);
                break;
            case OBF::StreetIndex::kNameEnFieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);
                localizedNames.insert(QLatin1String("en"), value);
                break;
            }
            case OBF::StreetIndex::kAttributeTagIdsFieldNumber:
            {
                const auto tgid = ObfReaderUtilities::readLength(cis);
                if (tgid < attributeTagsTable.count())
                    additionalTags << attributeTagsTable[tgid];
                break;
            }
            case OBF::StreetIndex::kAttributeValuesFieldNumber:
            {
                QString nm;
                ObfReaderUtilities::readQString(cis, nm);
                if (!additionalTags.isEmpty())
                {
                    QString tg = additionalTags.front();
                    additionalTags.pop_front();
                    if (tg.startsWith(QLatin1String("name:")))
                        localizedNames.insert(tg.mid(5), nm);
                }
                break;
            }
            case OBF::StreetIndex::kXFieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                position31.x = streetGroup->position31.x;
                if (d24 > 0)
                    position31.x += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    position31.x -= (static_cast<uint32_t>(-d24) << 7);
                break;
            }
            case OBF::StreetIndex::kYFieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                position31.y = streetGroup->position31.y;
                if (d24 > 0)
                    position31.y += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    position31.y -= (static_cast<uint32_t>(-d24) << 7);
                break;
            }
            case OBF::StreetIndex::kIdFieldNumber:
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&id));
                autogenerateId = false;
                break;
            case OBF::StreetIndex::kBuildingsFieldNumber:
                if (firstBuildingInnerOffset == 0)
                    firstBuildingInnerOffset = tagPos - streetOffset;
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::StreetIndex::kIntersectionsFieldNumber:
                if (firstIntersectionInnerOffset == 0)
                    firstIntersectionInnerOffset = tagPos - streetOffset;
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readBuildingsFromStreet(
    const ObfReader_P& reader,
    const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const Building> >* resultOut,
    const AreaI* const bbox31,
    const BuildingVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        //const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::StreetIndex::kBuildingsFieldNumber:
            {
                const auto offset = cis->CurrentPosition();
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<Building> building;
                readBuilding(reader, street, street->streetGroup, offset, building, bbox31, queryController);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                if (!visitor || visitor(building))
                {
                    if (resultOut)
                        resultOut->push_back(building);
                }
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readBuilding(
    const ObfReader_P& reader,
    const std::shared_ptr<const Street>& street,
    const std::shared_ptr<const StreetGroup>& streetGroup,
    const uint32_t buildingOffset,
    std::shared_ptr<Building>& outBuilding,
    const AreaI* const bbox31,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    Nullable<AreaI> buildingBBox31;

    auto autogenerateId = true;
    ObfObjectId id;
    QString nativeName;
    QHash<QString, QString> localizedNames;
    QString postcode;
    PointI position31;

    auto interpolation = Building::Interpolation::Disabled;
    auto interpolationInterval = 0;
    QString interpolationNativeName;
    QHash<QString, QString> interpolationLocalizedNames;
    PointI interpolationPosition31;
    
    QStringList additionalTags;
    QStringList attributeTagsTable;
    if (streetGroup && streetGroup->obfSection)
        attributeTagsTable = streetGroup->obfSection->attributeTagsTable;
    else if (street && street->obfSection)
            attributeTagsTable = street->obfSection->attributeTagsTable;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (bbox31 && buildingBBox31.isSet())
                {
                    const auto fitsBBox =
                        buildingBBox31->contains(*bbox31) ||
                        buildingBBox31->intersects(*bbox31) ||
                        bbox31->contains(*buildingBBox31);

                    if (!fitsBBox)
                        return;
                }

                if (street)
                    outBuilding.reset(new Building(street));
                else
                    outBuilding.reset(new Building(streetGroup));
                if (autogenerateId)
                    outBuilding->id = ObfObjectId::generateUniqueId(buildingOffset, streetGroup->obfSection);
                else
                    outBuilding->id = id;
                outBuilding->nativeName = nativeName;
                outBuilding->localizedNames = localizedNames;
                outBuilding->postcode = postcode;
                outBuilding->position31 = position31;

                outBuilding->interpolation = interpolation;
                outBuilding->interpolationInterval = interpolationInterval;
                outBuilding->interpolationNativeName = interpolationNativeName;
                outBuilding->interpolationLocalizedNames = interpolationLocalizedNames;
                outBuilding->interpolationPosition31 = interpolationPosition31;

                return;
            case OBF::BuildingIndex::kIdFieldNumber:
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&id));
                autogenerateId = false;
                break;
            case OBF::BuildingIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, nativeName);
                break;
            case OBF::BuildingIndex::kNameEnFieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);
                localizedNames.insert(QLatin1String("en"), value);
                break;
            }
            case OBF::BuildingIndex::kAttributeTagIdsFieldNumber:
            {
                const auto tgid = ObfReaderUtilities::readLength(cis);
                if (tgid < attributeTagsTable.count())
                    additionalTags << attributeTagsTable[tgid];
                break;
            }
            case OBF::BuildingIndex::kAttributeValuesFieldNumber:
            {
                QString nm;
                ObfReaderUtilities::readQString(cis, nm);
                if (!additionalTags.isEmpty())
                {
                    QString tg = additionalTags.front();
                    additionalTags.pop_front();
                    if (tg.startsWith(QLatin1String("name:")))
                        localizedNames.insert(tg.mid(5), nm);
                }
                break;
            }
            case OBF::BuildingIndex::kPostcodeFieldNumber:
            {
                ObfReaderUtilities::readQString(cis, postcode);
                break;
            }
            case OBF::BuildingIndex::kXFieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                position31.x = street ? street->position31.x : streetGroup->position31.x;
                if (d24 > 0)
                    position31.x += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    position31.x -= (static_cast<uint32_t>(-d24) << 7);
                break;
            }
            case OBF::BuildingIndex::kYFieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                position31.y = street ? street->position31.y : streetGroup->position31.y;
                if (d24 > 0)
                    position31.y += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    position31.y -= (static_cast<uint32_t>(-d24) << 7);

                if (buildingBBox31.isSet())
                    buildingBBox31->enlargeToInclude(position31);
                else
                    buildingBBox31 = AreaI(position31, position31);

                break;
            }
            case OBF::BuildingIndex::kInterpolationFieldNumber:
            {
                const auto value = ObfReaderUtilities::readSInt32(cis);
                if (value > 0)
                    interpolationInterval = value;
                else
                    interpolation = static_cast<Building::Interpolation>(value);
                break;
            }
            case OBF::BuildingIndex::kName2FieldNumber:
                ObfReaderUtilities::readQString(cis, interpolationNativeName);
                break;
            case OBF::BuildingIndex::kNameEn2FieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);
                interpolationLocalizedNames.insert(QLatin1String("en"), value);
                break;
            }
            case OBF::BuildingIndex::kX2FieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                interpolationPosition31.x = street ? street->position31.x : streetGroup->position31.x;
                if (d24 > 0)
                    interpolationPosition31.x += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    interpolationPosition31.x -= (static_cast<uint32_t>(-d24) << 7);
                break;
            }
            case OBF::BuildingIndex::kY2FieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                interpolationPosition31.y = street ? street->position31.y : streetGroup->position31.y;
                if (d24 > 0)
                    interpolationPosition31.y += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    interpolationPosition31.y -= (static_cast<uint32_t>(-d24) << 7);

                if (buildingBBox31.isSet())
                    buildingBBox31->enlargeToInclude(interpolationPosition31);
                else
                    buildingBBox31 = AreaI(interpolationPosition31, interpolationPosition31);

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readIntersectionsFromStreet(
    const ObfReader_P& reader,
    const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const StreetIntersection> >* resultOut,
    const AreaI* const bbox31,
    const IntersectionVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        //const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::StreetIndex::kIntersectionsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<StreetIntersection> streetIntersection;
                readStreetIntersection(reader, street, streetIntersection, bbox31, queryController);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                if (!visitor || visitor(streetIntersection))
                {
                    if (resultOut)
                        resultOut->push_back(streetIntersection);
                }
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readStreetIntersection(
    const ObfReader_P& reader,
    const std::shared_ptr<const Street>& street,
    std::shared_ptr<StreetIntersection>& outIntersection,
    const AreaI* const bbox31,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    QString nativeName;
    QHash<QString, QString> localizedNames;
    PointI position31;

    QStringList additionalTags;
    QStringList attributeTagsTable;
    if (street && street->obfSection)
        attributeTagsTable = street->obfSection->attributeTagsTable;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (bbox31 && !bbox31->contains(position31))
                    return;

                outIntersection.reset(new StreetIntersection(street));
                outIntersection->nativeName = nativeName;
                outIntersection->localizedNames = localizedNames;
                outIntersection->position31 = position31;

                return;
            case OBF::StreetIntersection::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, nativeName);
                break;
            case OBF::StreetIntersection::kNameEnFieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);

                localizedNames.insert(QLatin1String("en"), value);

                break;
            }
            case OBF::StreetIntersection::kAttributeTagIdsFieldNumber:
            {
                const auto tgid = ObfReaderUtilities::readLength(cis);
                if (tgid < attributeTagsTable.count())
                    additionalTags << attributeTagsTable[tgid];
                break;
            }
            case OBF::StreetIntersection::kAttributeValuesFieldNumber:
            {
                QString nm;
                ObfReaderUtilities::readQString(cis, nm);
                if (!additionalTags.isEmpty())
                {
                    QString tg = additionalTags.front();
                    additionalTags.pop_front();
                    if (tg.startsWith(QLatin1String("name:")))
                        localizedNames.insert(tg.mid(5), nm);
                }
                break;
            }
            case OBF::StreetIntersection::kIntersectedXFieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                position31.x = street->position31.x;
                if (d24 > 0)
                    position31.x += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    position31.x -= (static_cast<uint32_t>(-d24) << 7);
                break;
            }
            case OBF::StreetIntersection::kIntersectedYFieldNumber:
            {
                const auto d24 = ObfReaderUtilities::readSInt32(cis);
                position31.y = street->position31.y;
                if (d24 > 0)
                    position31.y += (static_cast<uint32_t>(d24) << 7);
                else if (d24 < 0)
                    position31.y -= (static_cast<uint32_t>(-d24) << 7);
                break;
            }
            break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}



void OsmAnd::ObfAddressSectionReader_P::readAddressesByName(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    const QString& query,
    const StringMatcherMode matcherMode,
    QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const bool includeStreets,
    const bool strictMatch,
    const ObfAddressSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    QVector<AddressReference> indexReferences;
    const OsmAnd::CollatorStringMatcher stringMatcher(query, matcherMode);

    for (;;)
    {
        //const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndAddressIndex::kNameIndexFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto oldLimit = cis->PushLimit(length);

                scanNameIndex(
                    reader,
                    query,
                    indexReferences,
                    bbox31,
                    streetGroupTypesFilter,
                    includeStreets,
                    strictMatch,
                    queryController);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                qSort(indexReferences.begin(), indexReferences.end(), ObfAddressSectionReader_P::dereferencedLessThan);
                uint32_t dataIndexOffsetStreet = 0;
                uint32_t dataIndexOffsetStreetGroup = 0;
                for (const auto& indexReference : constOf(indexReferences))
                {
                    std::shared_ptr<Address> address;
                    
                    if (indexReference.addressType == AddressNameIndexDataAtomType::Street)
                    {
                        if (dataIndexOffsetStreet == indexReference.dataIndexOffset)
                            continue;
                        else
                            dataIndexOffsetStreet = indexReference.dataIndexOffset;

                        std::shared_ptr<OsmAnd::StreetGroup> streetGroup;
                        {
                            cis->Seek(indexReference.containerIndexOffset);

                            gpb::uint32 length;
                            cis->ReadVarint32(&length);
                            const auto oldLimit = cis->PushLimit(length);

                            readStreetGroup(
                                reader,
                                section,
                                static_cast<ObfAddressStreetGroupType>(ObfAddressStreetGroupType::Unknown),
                                indexReference.containerIndexOffset,
                                streetGroup,
                                nullptr,
                                queryController);

                            ObfReaderUtilities::ensureAllDataWasRead(cis);
                            cis->PopLimit(oldLimit);
                        }
                        
                        if (!streetGroup)
                            continue;

                        std::shared_ptr<Street> street;
                        {
                            cis->Seek(indexReference.dataIndexOffset);
                            gpb::uint32 length;
                            cis->ReadVarint32(&length);
                            const auto oldLimit = cis->PushLimit(length);

                            readStreet(reader, streetGroup, indexReference.dataIndexOffset, street, bbox31, queryController);

                            ObfReaderUtilities::ensureAllDataWasRead(cis);
                            cis->PopLimit(oldLimit);
                        }

                        if (!street)
                            continue;
                        
                        if (!query.isNull())
                        {
                            bool accept = false;
                            accept = accept || stringMatcher.matches(street->nativeName);
                            for (const auto& localizedName : constOf(street->localizedNames))
                            {
                                accept = accept || stringMatcher.matches(localizedName);

                                if (accept)
                                    break;
                            }

                            if (!accept)
                                continue;
                        }
                        address = street;
                    }
                    else
                    {
                        if (dataIndexOffsetStreetGroup == indexReference.dataIndexOffset)
                            continue;
                        else
                            dataIndexOffsetStreetGroup = indexReference.dataIndexOffset;

                        std::shared_ptr<OsmAnd::StreetGroup> streetGroup;
                        {
                            cis->Seek(indexReference.dataIndexOffset);

                            gpb::uint32 length;
                            const auto offset = cis->CurrentPosition();
                            cis->ReadVarint32(&length);
                            const auto oldLimit = cis->PushLimit(length);

                            readStreetGroup(
                                reader,
                                section,
                                static_cast<ObfAddressStreetGroupType>(indexReference.addressType),
                                offset,
                                streetGroup,
                                bbox31,
                                queryController);

                            ObfReaderUtilities::ensureAllDataWasRead(cis);
                            cis->PopLimit(oldLimit);
                        }

                        if (!streetGroup)
                            continue;

                        if (!query.isNull())
                        {
                            bool accept = false;
                            accept = accept || stringMatcher.matches(streetGroup->nativeName);
                            for (const auto& localizedName : constOf(streetGroup->localizedNames))
                            {
                                accept = accept || stringMatcher.matches(localizedName);

                                if (accept)
                                    break;
                            }

                            if (!accept)
                                continue;
                        }
                        address = streetGroup;
                    }



                    if (address)
                    {
                        if (!visitor || visitor(address))
                        {
                            if (outAddresses)
                                outAddresses->push_back(address);
                        }
                    }

                    if (queryController && queryController->isAborted())
                        return;
                }

                cis->Skip(cis->BytesUntilLimit());
                return;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::scanNameIndex(
    const ObfReader_P& reader,
    const QString& query,
    QVector<AddressReference>& outAddressReferences,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const bool includeStreets,
    const bool strictMatch,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    uint32_t baseOffset;
    QVector<uint32_t> intermediateOffsets;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndAddressNameIndexData::kTableFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                baseOffset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                ObfReaderUtilities::scanIndexedStringTable(cis, query, intermediateOffsets, strictMatch);
                ObfReaderUtilities::ensureAllDataWasRead(cis);

                cis->PopLimit(oldLimit);

                if (intermediateOffsets.isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                break;
            }
            case OBF::OsmAndAddressNameIndexData::kAtomFieldNumber:
            {
                std::sort(intermediateOffsets);
                for (const auto& intermediateOffset : constOf(intermediateOffsets))
                {
                    const auto offset = baseOffset + intermediateOffset; 
                    cis->Seek(offset);
                    
                    gpb::uint32 length;
                    cis->ReadVarint32(&length);
                    const auto oldLimit = cis->PushLimit(length);

                    readNameIndexData(
                        reader,
                        offset,
                        outAddressReferences,
                        bbox31,
                        streetGroupTypesFilter,
                        includeStreets,
                        queryController);
                    ObfReaderUtilities::ensureAllDataWasRead(cis);

                    cis->PopLimit(oldLimit);
                }
                cis->Skip(cis->BytesUntilLimit());
                return;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readNameIndexData(
    const ObfReader_P& reader,
    const uint32_t baseOffset,
    QVector<AddressReference>& outAddressReferences,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const bool includeStreets,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndAddressNameIndexData_AddressNameIndexData::kAtomFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                readNameIndexDataAtom(
                    reader,
                    baseOffset,
                    outAddressReferences, 
                    bbox31,
                    streetGroupTypesFilter,
                    includeStreets,
                    queryController);
                ObfReaderUtilities::ensureAllDataWasRead(cis);

                cis->PopLimit(oldLimit);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::readNameIndexDataAtom(
    const ObfReader_P& reader,
    const uint32_t baseOffset,
    QVector<AddressReference>& outAddressReferences,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const bool includeStreets,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    AddressReference addressReference;
    bool add = true;
    
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                if (!add || !ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (addressReference.containerIndexOffset != 0)
                    addressReference.containerIndexOffset = baseOffset - addressReference.containerIndexOffset;
                if (addressReference.dataIndexOffset != 0)
                    addressReference.dataIndexOffset = baseOffset - addressReference.dataIndexOffset;
                outAddressReferences.push_back(addressReference);

                return;
            }
            case OBF::AddressNameIndexDataAtom::kNameFieldNumber:
            case OBF::AddressNameIndexDataAtom::kNameEnFieldNumber:
            {
                //NOTE: This value is not used, even if written
                QString value;
                ObfReaderUtilities::readQString(cis, value);
                break;
            }
            case OBF::AddressNameIndexDataAtom::kTypeFieldNumber:
            {
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&addressReference.addressType));

                auto shouldSkip = false;
                if (addressReference.addressType == AddressNameIndexDataAtomType::Street)
                    shouldSkip = shouldSkip || !includeStreets;
                else
                {
                    shouldSkip = shouldSkip ||
                        !streetGroupTypesFilter.isSet(static_cast<ObfAddressStreetGroupType>(addressReference.addressType));
                }
                if (shouldSkip)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                break;
            }
            case OBF::AddressNameIndexDataAtom::kShiftToIndexFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&addressReference.dataIndexOffset));
                break;
            case OBF::AddressNameIndexDataAtom::kShiftToCityIndexFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&addressReference.containerIndexOffset));
                break;
            case OBF::AddressNameIndexDataAtom::kXy16FieldNumber:
            {
                gpb::uint32 xy16;
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&xy16));
                int x = (xy16 >> 16) << 15;
                int y = (xy16 & ((1 << 16) - 1)) << 15;
                add = !bbox31 || bbox31->contains(x, y);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::loadStreetGroups(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const StreetGroup> >* resultOut,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const StreetGroupVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    for (auto block : section->cities)
    {
        if (block->type == (int)ObfAddressStreetGroupType::CityOrTown)
        {
            cis->Seek(block->offset);
            const auto oldLimit = cis->PushLimit(block->length);
            readStreetGroups(reader, section, resultOut, bbox31, streetGroupTypesFilter, visitor, queryController);
            ObfReaderUtilities::ensureAllDataWasRead(cis);
            cis->PopLimit(oldLimit);
        }
    }
}

void OsmAnd::ObfAddressSectionReader_P::loadStreetsFromGroup(
    const ObfReader_P& reader,
    const std::shared_ptr<const StreetGroup>& streetGroup,
    QList< std::shared_ptr<const Street> >* resultOut,
    const AreaI* const bbox31,
    const StreetVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(streetGroup->obfSection->offset);
    const auto oldLimit = cis->PushLimit(streetGroup->obfSection->length);
    const auto innerOffset = streetGroup->dataOffset - streetGroup->obfSection->offset;
    cis->Skip(innerOffset);

    {
        gpb::uint32 length;
        cis->ReadVarint32(&length);
        const auto oldLimit = cis->PushLimit(length);

        readStreetsFromGroup(reader, streetGroup, resultOut, bbox31, visitor, queryController);

        ObfReaderUtilities::ensureAllDataWasRead(cis);
        cis->PopLimit(oldLimit);
    }

    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfAddressSectionReader_P::loadBuildingsFromStreet(
    const ObfReader_P& reader,
    const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const Building> >* resultOut,
    const AreaI* const bbox31,
    const BuildingVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(street->offset);

    
    gpb::uint32 length;
    cis->ReadVarint32(&length);
    
    const auto oldLimit = cis->PushLimit(length);
    cis->Skip(street->firstBuildingInnerOffset - (cis->CurrentPosition() - street->offset));

    readBuildingsFromStreet(reader, street, resultOut, bbox31, visitor, queryController);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
    
}

void OsmAnd::ObfAddressSectionReader_P::loadIntersectionsFromStreet(
    const ObfReader_P& reader,
    const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const StreetIntersection> >* resultOut,
    const AreaI* const bbox31,
    const IntersectionVisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    cis->Skip(street->offset);

    gpb::uint32 length;
    cis->ReadVarint32(&length);
    const auto oldLimit = cis->PushLimit(length);
    cis->Skip(street->firstIntersectionInnerOffset - (cis->CurrentPosition() - street->offset));

    readIntersectionsFromStreet(reader, street, resultOut, bbox31, visitor, queryController);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);


}

void OsmAnd::ObfAddressSectionReader_P::scanAddressesByName(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    const QString& query,
    const StringMatcherMode matcherMode,
    QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
    const AreaI* const bbox31,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
    const bool includeStreets,
    const bool strictMatch,
    const ObfAddressSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    auto oldLimit = cis->PushLimit(section->length);
    cis->Skip(section->nameIndexInnerOffset);

    readAddressesByName(
        reader,
        section,
        query,
        matcherMode,
        outAddresses,
        bbox31,
        streetGroupTypesFilter,
        includeStreets,
        strictMatch,
        visitor,
        queryController);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}
