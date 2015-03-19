#include "ObfPoiSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfReader_P.h"
#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionInfo_P.h"
#include "Amenity.h"
#include "ObfReaderUtilities.h"
#include "IQueryController.h"
#include "Utilities.h"

OsmAnd::ObfPoiSectionReader_P::ObfPoiSectionReader_P()
{
}

OsmAnd::ObfPoiSectionReader_P::~ObfPoiSectionReader_P()
{
}

void OsmAnd::ObfPoiSectionReader_P::read(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfPoiSectionInfo>& section)
{
    const auto cis = reader.getCodedInputStream().get();

    for (;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndPoiIndex::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, section->name);
                break;
            case OBF::OsmAndPoiIndex::kBoundariesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                auto oldLimit = cis->PushLimit(length);

                readBoundaries(reader, section);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndPoiIndex::kCategoriesTableFieldNumber:
                if (!section->hasCategories)
                {
                    section->hasCategories = true;
                    section->firstCategoryInnerOffset = tagPos - section->offset;
                }
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::OsmAndPoiIndex::kNameIndexFieldNumber:
                if (!section->hasNameIndex)
                {
                    section->hasNameIndex = true;
                    section->hasNameIndex = tagPos - section->offset;
                }
            case OBF::OsmAndPoiIndex::kSubtypesTableFieldNumber:
            case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
                cis->Skip(cis->BytesUntilLimit());
                return;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readBoundaries(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfPoiSectionInfo>& section)
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
        case OBF::OsmAndTileBox::kLeftFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->area31.left()));
            break;
        case OBF::OsmAndTileBox::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->area31.right()));
            break;
        case OBF::OsmAndTileBox::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->area31.top()));
            break;
        case OBF::OsmAndTileBox::kBottomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->area31.bottom()));
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readCategories(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfPoiSectionCategories>& categories)
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
        case OBF::OsmAndPoiIndex::kCategoriesTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);
                
                QString mainCategory;
                QStringList subcategories;
                readCategoriesEntry(reader, mainCategory, subcategories);
                
                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                categories->mainCategories.push_back(qMove(mainCategory));
                categories->subCategories.push_back(qMove(subcategories));
            }
            break;
        case OBF::OsmAndPoiIndex::kNameIndexFieldNumber:
        case OBF::OsmAndPoiIndex::kSubtypesTableFieldNumber:
        case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
        case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readCategoriesEntry(
    const ObfReader_P& reader,
    QString& outMainCategory,
    QStringList& outSubcategories)
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
            case OBF::OsmAndCategoryTable::kCategoryFieldNumber:
                ObfReaderUtilities::readQString(cis, outMainCategory);
                break;
            case OBF::OsmAndCategoryTable::kSubcategoriesFieldNumber:
            {
                QString name;
                ObfReaderUtilities::readQString(cis, name);
                outSubcategories.push_back(qMove(name));
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::ensureCategoriesLoaded(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section)
{
    if (!section->hasCategories)
        return;

    if (section->_p->_categoriesLoaded.loadAcquire() == 0)
    {
        QMutexLocker scopedLocker(&section->_p->_categoriesLoadMutex);
        if (!section->_p->_categories)
        {
            const auto cis = reader.getCodedInputStream().get();

            cis->Seek(section->offset);
            const auto oldLimit = cis->PushLimit(section->length);
            cis->Skip(section->firstCategoryInnerOffset);

            const std::shared_ptr<ObfPoiSectionCategories> categories(new ObfPoiSectionCategories());
            readCategories(reader, categories);
            section->_p->_categories = categories;

            ObfReaderUtilities::ensureAllDataWasRead(cis);
            cis->PopLimit(oldLimit);
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readAmenities(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const IQueryController* const controller)
{
    const auto cis = reader.getCodedInputStream().get();
    
    QVector<uint32_t> dataBoxesOffsets;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                readDataOffsetsFromTiles(
                    reader,
                    dataBoxesOffsets,
                    MinZoomLevel,
                    TileId(),
                    minZoom,
                    maxZoom,
                    bbox31,
                    categoriesFilter);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                if (controller && controller->isAborted())
                    return;

                break;
            }
            case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            {
                std::sort(dataBoxesOffsets);

                auto pDataOffset = dataBoxesOffsets.constData();
                const auto dataBoxesCount = dataBoxesOffsets.size();
                for (auto dataBoxIndex = 0; dataBoxIndex < dataBoxesCount; dataBoxIndex++)
                {
                    const auto dataOffset = *(pDataOffset++);

                    cis->Seek(section->offset + dataOffset);
                    const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                    const auto offset = cis->CurrentPosition();
                    const auto oldLimit = cis->PushLimit(length);

                    readAmenitiesDataBox(
                        reader,
                        section,
                        outAmenities,
                        minZoom,
                        maxZoom,
                        bbox31,
                        categoriesFilter,
                        visitor,
                        controller);

                    ObfReaderUtilities::ensureAllDataWasRead(cis);
                    cis->PopLimit(oldLimit);
                    if (controller && controller->isAborted())
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

void OsmAnd::ObfPoiSectionReader_P::readDataOffsetsFromTiles(
    const ObfReader_P& reader,
    QVector<uint32_t>& outDataOffsets,
    const ZoomLevel parentZoom,
    const TileId parentTileId,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter)
{
    const auto cis = reader.getCodedInputStream().get();

    gpb::uint32 deltaZoom = 0;
    auto zoom = MinZoomLevel;
    auto tileId = TileId::fromXY(0, 0);

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndPoiBox::kZoomFieldNumber:
            {
                cis->ReadVarint32(&deltaZoom);

                zoom = static_cast<ZoomLevel>(static_cast<gpb::uint32>(parentZoom)+deltaZoom);
                if (zoom > maxZoom)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                break;
            }
            case OBF::OsmAndPoiBox::kLeftFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                tileId.x = (parentTileId.x << deltaZoom) + d;
                break;
            }
            case OBF::OsmAndPoiBox::kTopFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                tileId.y = (parentTileId.y << deltaZoom) + d;

                if (bbox31)
                {
                    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
                    const auto shouldSkip =
                        !bbox31->contains(tileBBox31) &&
                        !tileBBox31.contains(*bbox31) &&
                        !bbox31->intersects(tileBBox31);
                    if (shouldSkip)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                break;
            }
            case OBF::OsmAndPoiBox::kCategoriesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                if (!categoriesFilter)
                {
                    cis->Skip(length);
                    break;
                }
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                const auto hasMatchingContent = tileContainsAnyCategory(reader, *categoriesFilter);

                cis->PopLimit(oldLimit);

                if (!hasMatchingContent)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                break;
            }
            case OBF::OsmAndPoiBox::kSubBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                readDataOffsetsFromTiles(reader, outDataOffsets, zoom, tileId, minZoom, maxZoom, bbox31, categoriesFilter);
                ObfReaderUtilities::ensureAllDataWasRead(cis);

                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndPoiBox::kShiftToDataFieldNumber:
            {
                const auto dataOffset = ObfReaderUtilities::readBigEndianInt(cis);

                if (zoom < minZoom)
                    break;

                outDataOffsets.push_back(dataOffset);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

bool OsmAnd::ObfPoiSectionReader_P::tileContainsAnyCategory(
    const ObfReader_P& reader,
    const QSet<ObfPoiCategoryId>& categories)
{
    const auto cis = reader.getCodedInputStream().get();
    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return false;

                return false;
            case OBF::OsmAndPoiCategories::kCategoriesFieldNumber:
            {
                ObfPoiCategoryId id;
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&id));
                
                if (categories.contains(id))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return true;
                }
                break;
            }
            //			case OsmandOdb.OsmAndPoiCategories.SUBCATEGORIES_FIELD_NUMBER:
            //				int subcatvl = codedIS.readUInt32();
            //				if(req.poiTypeFilter.filterSubtypes()) {
            //					subType.setLength(0);
            //					PoiSubType pt = region.getSubtypeFromId(subcatvl, subType);
            //					if(pt != null && req.poiTypeFilter.accept(pt.name, subType.toString())) {
            //						codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
            //						return true;
            //					}
            //				}
            //				break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readAmenitiesDataBox(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const IQueryController* const controller)
{
    const auto cis = reader.getCodedInputStream().get();

    auto zoom = InvalidZoomLevel;
    TileId tileId;
    bool firstAmenityRead = false;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndPoiBoxData::kZoomFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);

                zoom = static_cast<ZoomLevel>(value);
                break;
            }
            case OBF::OsmAndPoiBoxData::kXFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);

                tileId.x = value;
                break;
            }
            case OBF::OsmAndPoiBoxData::kYFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);

                tileId.y = value;
                break;
            }
            case OBF::OsmAndPoiBoxData::kPoiDataFieldNumber:
            {
                if (!firstAmenityRead)
                {
                    bool rejectBox = false;
                    rejectBox = rejectBox || (zoom < minZoom || zoom > maxZoom);
                    if (bbox31)
                    {
                        const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
                        const auto shouldSkip =
                            !bbox31->contains(tileBBox31) &&
                            !tileBBox31.contains(*bbox31) &&
                            !bbox31->intersects(tileBBox31);

                        rejectBox = rejectBox || shouldSkip;
                    }

                    if (rejectBox)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<const Amenity> amenity;
                readAmenity(reader, section, amenity, zoom, tileId, bbox31, categoriesFilter, controller);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                if (!amenity)
                    break;

                if (!visitor || visitor(amenity))
                {
                    if (outAmenities)
                        outAmenities->push_back(qMove(amenity));
                }
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readAmenity(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    std::shared_ptr<const Amenity>& outAmenity,
    const ZoomLevel zoom,
    const TileId boxTileId,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const IQueryController* const controller)
{
    const auto cis = reader.getCodedInputStream().get();
    const auto baseOffset = cis->CurrentPosition();

    bool autogenerateId = true;
    std::shared_ptr<Amenity> amenity;
    PointI position31;
    QSet<ObfPoiCategoryId> categories;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (!amenity)
                    amenity.reset(new Amenity(section));

                amenity->position31 = position31;
                amenity->categories = categories;
                if (autogenerateId)
                    amenity->id = ObfObjectId::generateUniqueId(baseOffset, section);
                outAmenity = amenity;
                return;
            }
            case OBF::OsmAndPoiBoxDataAtom::kDxFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                position31.x = ((boxTileId.x << (24 - zoom)) + d) << 7;
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kDyFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                position31.y = ((boxTileId.y << (24 - zoom)) + d) << 7;

                if (bbox31 && !bbox31->contains(position31))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kCategoriesFieldNumber:
            {
                ObfPoiCategoryId categoryId;
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&categoryId));
                categories.insert(categoryId);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kSubcategoriesFieldNumber:
            {
                if (categoriesFilter && categories.intersect(*categoriesFilter).isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }

                if (!amenity)
                    amenity.reset(new Amenity(section));
                /*int subtypev = codedIS.readUInt32();
                retValue.setLength(0);
                PoiSubType st = region.getSubtypeFromId(subtypev, retValue);
                if (st != null) {
                    am.setAdditionalInfo(st.name, retValue.toString());
                }*/
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kNameFieldNumber:
                if (!amenity)
                    amenity.reset(new Amenity(section));
                ObfReaderUtilities::readQString(cis, amenity->nativeName);
                break;
            case OBF::OsmAndPoiBoxDataAtom::kNameEnFieldNumber:
            {
                if (!amenity)
                    amenity.reset(new Amenity(section));

                QString name;
                ObfReaderUtilities::readQString(cis, name);
                amenity->localizedNames.insert(QLatin1String("en"), name);

                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kIdFieldNumber:
                if (!amenity)
                    amenity.reset(new Amenity(section));

                gpb::uint64 rawId;
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&rawId));
                amenity->id = ObfObjectId::generateUniqueId(rawId, baseOffset, section);

                autogenerateId = false;
                break;
            //repeated uint32 textCategories = 14; // v1.7
            //repeated string textValues = 15;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::loadCategories(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    std::shared_ptr<const ObfPoiSectionCategories>& outCategories,
    const IQueryController* const controller)
{
    if (!section->hasCategories)
        return;

    ensureCategoriesLoaded(reader, section);
    outCategories = section->getCategories();
}

void OsmAnd::ObfPoiSectionReader_P::loadAmenities(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const IQueryController* const controller)
{
    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    auto oldLimit = cis->PushLimit(section->length);

    readAmenities(reader, section, outAmenities, minZoom, maxZoom, bbox31, categoriesFilter, visitor, controller);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}
