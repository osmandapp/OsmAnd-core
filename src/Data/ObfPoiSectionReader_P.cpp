#include "ObfPoiSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include "QtCommon.h"

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
                if (section->firstCategoryInnerOffset == 0)
                    section->firstCategoryInnerOffset = tagPos - section->offset;
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::OsmAndPoiIndex::kNameIndexFieldNumber:
                section->nameIndexInnerOffset = tagPos - section->offset;
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
            case OBF::OsmAndPoiIndex::kSubtypesTableFieldNumber:
                section->subtypesInnerOffset = tagPos - section->offset;
                ObfReaderUtilities::skipBlockWithLength(cis);
                break;
            case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
                if (section->firstBoxInnerOffset == 0)
                    section->firstBoxInnerOffset = tagPos - section->offset;
                cis->Skip(cis->BytesUntilLimit());
                return;
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

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
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

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
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
                break;
            }
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

            section->_p->_categoriesLoaded.storeRelease(1);
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readSubtypes(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfPoiSectionSubtypes>& subtypes)
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
            case OBF::OsmAndPoiIndex::kSubtypesTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                readSubtypesStructure(reader, subtypes);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
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

void OsmAnd::ObfPoiSectionReader_P::readSubtypesStructure(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfPoiSectionSubtypes>& subtypes)
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
            case OBF::OsmAndSubtypesTable::kSubtypesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                const std::shared_ptr<ObfPoiSectionSubtype> subtype(new ObfPoiSectionSubtype());
                readSubtype(reader, subtype);
                subtypes->subtypes.push_back(subtype);

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

void OsmAnd::ObfPoiSectionReader_P::readSubtype(
    const ObfReader_P& reader,
    const std::shared_ptr<ObfPoiSectionSubtype>& subtype)
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
            case OBF::OsmAndPoiSubtype::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, subtype->name);
                break;
            case OBF::OsmAndPoiSubtype::kTagnameFieldNumber:
                ObfReaderUtilities::readQString(cis, subtype->tagName);
                break;
            case OBF::OsmAndPoiSubtype::kIsTextFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                subtype->isText = (value != 0);
                break;
            }
            case OBF::OsmAndPoiSubtype::kFrequencyFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&subtype->frequency));
                break;
            //case OBF::OsmAndPoiSubtype::kSubtypeValuesSizeFieldNumber:
            //    break;
            case OBF::OsmAndPoiSubtype::kSubtypeValueFieldNumber:
            {
                QString value;
                ObfReaderUtilities::readQString(cis, value);
                subtype->possibleValues.push_back(value);
                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::ensureSubtypesLoaded(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section)
{
    if (section->_p->_subtypesLoaded.loadAcquire() == 0)
    {
        QMutexLocker scopedLocker(&section->_p->_subtypesLoadMutex);
        if (!section->_p->_subtypes)
        {
            const auto cis = reader.getCodedInputStream().get();

            cis->Seek(section->offset);
            const auto oldLimit = cis->PushLimit(section->length);
            cis->Skip(section->subtypesInnerOffset);

            const std::shared_ptr<ObfPoiSectionSubtypes> subtypes(new ObfPoiSectionSubtypes());
            readSubtypes(reader, subtypes);
            section->_p->_subtypes = subtypes;

            ObfReaderUtilities::ensureAllDataWasRead(cis);
            cis->PopLimit(oldLimit);

            section->_p->_subtypesLoaded.storeRelease(1);
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

                scanTiles(
                    reader,
                    dataBoxesOffsets,
                    MinZoomLevel,
                    TileId::zero(),
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
                        QString::null,
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

void OsmAnd::ObfPoiSectionReader_P::scanTiles(
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
    auto tileId = TileId::zero();

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

                const auto hasMatchingContent = scanTileForMatchingCategories(reader, *categoriesFilter);

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

                scanTiles(reader, outDataOffsets, zoom, tileId, minZoom, maxZoom, bbox31, categoriesFilter);
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

bool OsmAnd::ObfPoiSectionReader_P::scanTileForMatchingCategories(
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
    const QString& query,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const IQueryController* const controller)
{
    const auto cis = reader.getCodedInputStream().get();

    auto zoom = InvalidZoomLevel;
    auto tileId = TileId::zero();
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
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&zoom));
                break;
            case OBF::OsmAndPoiBoxData::kXFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&tileId.x));
                break;
            case OBF::OsmAndPoiBoxData::kYFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&tileId.y));
                break;
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
                readAmenity(reader, section, amenity, query, zoom, tileId, bbox31, categoriesFilter, controller);

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
    const QString& query,
    const ZoomLevel zoom,
    const TileId boxTileId,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const IQueryController* const controller)
{
    const auto cis = reader.getCodedInputStream().get();
    const auto baseOffset = cis->CurrentPosition();

    ObfObjectId id;
    bool autogenerateId = true;
    std::shared_ptr<Amenity> amenity;
    PointI position31;
    QString nativeName;
    QHash<QString, QString> localizedNames;
    QList<ObfPoiCategoryId> categories;
    QVector<int> textValueSubtypeIndices;
    QHash<int, QVariant> intValues;
    QHash<int, QVariant> stringValues;
    auto categoriesFilterChecked = false;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (!query.isNull())
                {
                    bool accept = false;
                    accept = accept || nativeName.contains(query, Qt::CaseInsensitive);
                    for (const auto& localizedName : constOf(localizedNames))
                    {
                        accept = accept || localizedName.contains(query, Qt::CaseInsensitive);

                        if (accept)
                            break;
                    }

                    if (!accept)
                        return;
                }

                if (!categoriesFilterChecked &&
                    categoriesFilter &&
                    categories.toSet().intersect(*categoriesFilter).isEmpty())
                {
                    return;
                }

                if (!amenity)
                    amenity.reset(new Amenity(section));

                amenity->nativeName = qMove(nativeName);
                amenity->localizedNames = qMove(localizedNames);
                amenity->position31 = position31;
                amenity->categories = qMove(categories);
                if (autogenerateId)
                    amenity->id = ObfObjectId::generateUniqueId(baseOffset, section);
                else
                    amenity->id = id;
                amenity->values = detachedOf(intValues).unite(stringValues);
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
                categories.push_back(categoryId);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kSubcategoriesFieldNumber:
            {
                if (!categoriesFilterChecked &&
                    categoriesFilter &&
                    categories.toSet().intersect(*categoriesFilter).isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                categoriesFilterChecked = true;

                gpb::uint32 rawValue;
                cis->ReadVarint32(&rawValue);

                const auto subtypeIndex = ((rawValue & 0x1) == 0x1)
                    ? ((rawValue >> 1) & 0xFFFF)
                    : ((rawValue >> 1) & 0x1F);
                const auto intValue = ((rawValue & 0x1) == 0x1)
                    ? (rawValue >> 17)
                    : (rawValue >> 6);

                intValues.insert(subtypeIndex, QVariant(intValue));
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, nativeName);
                break;
            case OBF::OsmAndPoiBoxDataAtom::kNameEnFieldNumber:
            {
                QString name;
                ObfReaderUtilities::readQString(cis, name);
                localizedNames.insert(QLatin1String("en"), name);

                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kIdFieldNumber:
            {
                gpb::uint64 rawId;
                cis->ReadVarint64(&rawId);
                id = ObfObjectId::generateUniqueId(rawId, baseOffset, section);

                autogenerateId = false;
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kTextCategoriesFieldNumber:
            {
                gpb::uint32 rawValue;
                cis->ReadVarint32(&rawValue);
                const auto subtypeIndex = ((rawValue & 0x1) == 0x1)
                    ? ((rawValue >> 1) & 0xFFFF)
                    : ((rawValue >> 1) & 0x1F);

                textValueSubtypeIndices.push_back(subtypeIndex);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kTextValuesFieldNumber:
            {
                QString valueString;
                ObfReaderUtilities::readQString(cis, valueString);
                if (stringValues.size() >= textValueSubtypeIndices.size())
                    break;

                const auto subtypeIndex = textValueSubtypeIndices[stringValues.size()];
                stringValues.insert(subtypeIndex, valueString);

                break;
            }
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readAmenitiesByName(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    const QString& query,
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
            case OBF::OsmAndPoiIndex::kNameIndexFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                scanNameIndex(
                    reader,
                    query,
                    dataBoxesOffsets,
                    minZoom,
                    maxZoom,
                    bbox31);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
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
                        query,
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

void OsmAnd::ObfPoiSectionReader_P::scanNameIndex(
    const ObfReader_P& reader,
    const QString& query,
    QVector<uint32_t>& outDataOffsets,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31)
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
            case OBF::OsmAndPoiNameIndex::kTableFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                baseOffset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                ObfReaderUtilities::scanIndexedStringTable(cis, query, intermediateOffsets);
                ObfReaderUtilities::ensureAllDataWasRead(cis);

                cis->PopLimit(oldLimit);

                if (intermediateOffsets.isEmpty())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
                break;
            }
            case OBF::OsmAndPoiNameIndex::kDataFieldNumber:
            {
                std::sort(intermediateOffsets);
                for (const auto& intermediateOffset : constOf(intermediateOffsets))
                {
                    cis->Seek(baseOffset + intermediateOffset);

                    gpb::uint32 length;
                    cis->ReadVarint32(&length);
                    const auto oldLimit = cis->PushLimit(length);

                    readNameIndexData(reader, outDataOffsets, minZoom, maxZoom, bbox31);
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

void OsmAnd::ObfPoiSectionReader_P::readNameIndexData(
    const ObfReader_P& reader,
    QVector<uint32_t>& outDataOffsets,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31)
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
            case OBF::OsmAndPoiNameIndex_OsmAndPoiNameIndexData::kAtomsFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto oldLimit = cis->PushLimit(length);

                readNameIndexDataAtom(reader, outDataOffsets, minZoom, maxZoom, bbox31);
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

void OsmAnd::ObfPoiSectionReader_P::readNameIndexDataAtom(
    const ObfReader_P& reader,
    QVector<uint32_t>& outDataOffsets,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31)
{
    const auto cis = reader.getCodedInputStream().get();

    auto tileId = TileId::zero();
    auto zoom = MinZoomLevel;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                return;
            case OBF::OsmAndPoiNameIndexDataAtom::kXFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&tileId.x));
                break;
            case OBF::OsmAndPoiNameIndexDataAtom::kYFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&tileId.y));
                break;
            case OBF::OsmAndPoiNameIndexDataAtom::kZoomFieldNumber:
                cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&zoom));
                break;
            case OBF::OsmAndPoiNameIndexDataAtom::kShiftToFieldNumber:
            {
                const auto dataOffset = ObfReaderUtilities::readBigEndianInt(cis);

                if (zoom > maxZoom || zoom < minZoom)
                    break;

                bool accept = true;
                if (bbox31)
                {
                    PointI position31;
                    position31.x = tileId.x << (31 - zoom);
                    position31.y = tileId.y << (31 - zoom);
                    accept = bbox31->contains(position31);
                }

                if (accept)
                    outDataOffsets.push_back(dataOffset);
                break;
            }
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
    ensureCategoriesLoaded(reader, section);
    outCategories = section->getCategories();
}

void OsmAnd::ObfPoiSectionReader_P::loadSubtypes(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
    const IQueryController* const controller)
{
    ensureSubtypesLoaded(reader, section);
    outSubtypes = section->getSubtypes();
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
    ensureCategoriesLoaded(reader, section);
    ensureSubtypesLoaded(reader, section);

    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    auto oldLimit = cis->PushLimit(section->length);
    cis->Skip(section->firstBoxInnerOffset);

    readAmenities(reader, section, outAmenities, minZoom, maxZoom, bbox31, categoriesFilter, visitor, controller);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSectionReader_P::scanAmenitiesByName(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    const QString& query,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const IQueryController* const controller)
{
    ensureCategoriesLoaded(reader, section);
    ensureSubtypesLoaded(reader, section);

    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    auto oldLimit = cis->PushLimit(section->length);
    cis->Skip(section->nameIndexInnerOffset);

    readAmenitiesByName(reader, section, query, outAmenities, minZoom, maxZoom, bbox31, categoriesFilter, visitor, controller);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}
