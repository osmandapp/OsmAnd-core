#include "ObfPoiSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include "QtCommon.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include "restore_internal_warnings.h"

#include "ObfReader_P.h"
#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionInfo_P.h"
#include "Amenity.h"
#include "ObfReaderUtilities.h"
#include "IQueryController.h"
#include "Utilities.h"
#include "CollatorStringMatcher.h"
#include <Logging.h>
#include <OsmAndCore/ICU.h>

const int BUCKET_SEARCH_BY_NAME = 5;
const int BASE_POI_SHIFT = 7;
const int FINAL_POI_SHIFT = 5;
const int BASE_POI_ZOOM = 31 - BASE_POI_SHIFT;
const int FINAL_POI_ZOOM = 31 - FINAL_POI_SHIFT;

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

                ObfReaderUtilities::readTileBox(cis, section->area31);

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

                if (subtype->tagName == QLatin1String("opening_hours"))
                {
                    subtypes->openingHoursSubtypeIndex = subtypes->subtypes.size() - 1;
                    subtypes->openingHoursSubtype = subtype;
                }
                else if (subtype->tagName == QLatin1String("website"))
                {
                    subtypes->websiteSubtypeIndex = subtypes->subtypes.size() - 1;
                    subtypes->websiteSubtype = subtype;
                }
                else if (subtype->tagName == QLatin1String("phone"))
                {
                    subtypes->phoneSubtypeIndex = subtypes->subtypes.size() - 1;
                    subtypes->phoneSubtype = subtype;
                }
                else if (subtype->tagName == QLatin1String("description"))
                {
                    subtypes->descriptionSubtypeIndex = subtypes->subtypes.size() - 1;
                    subtypes->descriptionSubtype = subtype;
                }

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

    bool tagNamePresent = false;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                if (!tagNamePresent)
                    subtype->tagName = subtype->name;

                return;
            case OBF::OsmAndPoiSubtype::kNameFieldNumber:
                ObfReaderUtilities::readQString(cis, subtype->name);
                break;
            case OBF::OsmAndPoiSubtype::kTagnameFieldNumber:
                ObfReaderUtilities::readQString(cis, subtype->tagName);
                tagNamePresent = true;
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
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter,
    const ZoomLevel zoomFilter,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    QMap<uint32_t, uint64_t> dataBoxesOffsetsMap;
    std::shared_ptr<QSet<uint64_t>> tilesToSkip = nullptr;
    if (zoomFilter >= 0 && zoomFilter < 16) {
        tilesToSkip = std::make_shared<QSet<uint64_t>>();
    }

    const auto zoomToSkip = zoomFilter == InvalidZoomLevel
        ? ZoomLevel31
        : static_cast<ZoomLevel>(zoomFilter + ZoomToSkipFilter);

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
                    dataBoxesOffsetsMap,
                    tilesToSkip,
                    MinZoomLevel,
                    TileId::zero(),
                    bbox31,
                    tileFilter,
                    zoomFilter,
                    categoriesFilter);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                if (queryController && queryController->isAborted())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }

                break;
            }
            case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            {
                if (tilesToSkip)
                    tilesToSkip->clear();

                for (const auto& dataBoxOffsetMapEntry : rangeOf(constOf(dataBoxesOffsetsMap)))
                {
                    const auto dataOffset = dataBoxOffsetMapEntry.key();
                    auto tileValue = dataBoxOffsetMapEntry.value();

                    if (tilesToSkip && zoomFilter != InvalidZoomLevel && tileValue != std::numeric_limits<uint64_t>::max())
                    {
                        const auto shift = ZoomToSkipFilterRead - ZoomToSkipFilter;
                        const auto dx = tileValue >> ZoomToSkipFilterRead;
                        const auto dy = tileValue - (dx << ZoomToSkipFilterRead);
                        tileValue = ((dx >> shift) << ZoomToSkipFilter) | (dy >> shift);
                        if (tileValue != std::numeric_limits<uint64_t>::max() && tilesToSkip->contains(tileValue))
                            continue;
                    }

                    cis->Seek(section->offset + dataOffset);
                    const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                    const auto offset = cis->CurrentPosition();
                    const auto oldLimit = cis->PushLimit(length);

                    const auto atLeastOneAccepted = readAmenitiesDataBox(
                        reader,
                        section,
                        outAmenities,
                        QString::null,
                        bbox31,
                        tileFilter,
                        zoomToSkip,
                        tilesToSkip,
                        categoriesFilter,
                        visitor,
                        queryController);

                    if (tilesToSkip && zoomFilter != InvalidZoomLevel && atLeastOneAccepted)
                        tilesToSkip->insert(tileValue);

                    ObfReaderUtilities::ensureAllDataWasRead(cis);
                    cis->PopLimit(oldLimit);
                    if (queryController && queryController->isAborted())
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
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

bool OsmAnd::ObfPoiSectionReader_P::scanTiles(
    const ObfReader_P& reader,
    QMap<uint32_t, uint64_t>& outDataOffsetsMap,
    const std::shared_ptr<QSet<uint64_t>> tilesToSkip,
    const ZoomLevel parentZoom,
    const TileId parentTileId,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter,
    const ZoomLevel zoomFilter,
    const QSet<ObfPoiCategoryId>* const categoriesFilter)
{
    const auto cis = reader.getCodedInputStream().get();

    const auto zoomToSkip = zoomFilter == InvalidZoomLevel
        ? ZoomLevel31
        : static_cast<ZoomLevel>(zoomFilter + ZoomToSkipFilterRead);

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
                    return false;

                return true;
            case OBF::OsmAndPoiBox::kZoomFieldNumber:
            {
                cis->ReadVarint32(&deltaZoom);

                zoom = static_cast<ZoomLevel>(static_cast<gpb::uint32>(parentZoom)+deltaZoom);
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

                bool rejectBox = false;

                //if (!rejectBox && tileFilter)
                //    rejectBox = !tileFilter(tileId, zoom);

                if (!rejectBox && bbox31)
                {
                    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
                    rejectBox =
                        !bbox31->contains(tileBBox31) &&
                        !tileBBox31.contains(*bbox31) &&
                        !bbox31->intersects(tileBBox31);
                }

                if (rejectBox)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return false;
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
                    return false;
                }
                break;
            }
            case OBF::OsmAndPoiBox::kSubBoxesFieldNumber:
            {
                const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                const auto wasAccepted = scanTiles(
                    reader,
                    outDataOffsetsMap,
                    tilesToSkip,
                    zoom,
                    tileId,
                    bbox31,
                    tileFilter,
                    zoomFilter,
                    categoriesFilter);
                ObfReaderUtilities::ensureAllDataWasRead(cis);

                cis->PopLimit(oldLimit);

                if (tilesToSkip && zoomFilter != InvalidZoomLevel && zoom >= zoomToSkip && wasAccepted)
                {
                    const auto tileValue =
                        ((static_cast<uint64_t>(tileId.x) >> (zoom - zoomToSkip)) << zoomToSkip) |
                        (static_cast<uint64_t>(tileId.y) >> (zoom - zoomToSkip));
                    if (tilesToSkip->contains(tileValue))
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return true;
                    }
                }

                break;
            }
            case OBF::OsmAndPoiBox::kShiftToDataFieldNumber:
            {
                /*
                boolean read = true;
                if (req.tiles != null) {
                    long zx = x << (SearchRequest.ZOOM_TO_SEARCH_POI - zoom);
                    long zy = y << (SearchRequest.ZOOM_TO_SEARCH_POI - zoom);
                    read = req.tiles.contains((zx << SearchRequest.ZOOM_TO_SEARCH_POI) + zy);
                }*/

                bool read = true;
                if (tileFilter)
                    read = tileFilter(tileId, zoom);

                const auto dataOffset = ObfReaderUtilities::readBigEndianInt(cis);

                if (read)
                {
                    if (tilesToSkip && zoomFilter != InvalidZoomLevel && zoom >= zoomToSkip)
                    {
                        const auto tileValue =
                        ((static_cast<uint64_t>(tileId.x) >> (zoom - zoomToSkip)) << zoomToSkip) |
                        (static_cast<uint64_t>(tileId.y) >> (zoom - zoomToSkip));
                        outDataOffsetsMap.insert(dataOffset, tileValue);
                        tilesToSkip->insert(tileValue);
                    }
                    else
                    {
                        outDataOffsetsMap.insert(dataOffset, std::numeric_limits<uint64_t>::max());
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
            //            case OsmandOdb.OsmAndPoiCategories.SUBCATEGORIES_FIELD_NUMBER:
            //                int subcatvl = codedIS.readUInt32();
            //                if(req.poiTypeFilter.filterSubtypes()) {
            //                    subType.setLength(0);
            //                    PoiSubType pt = region.getSubtypeFromId(subcatvl, subType);
            //                    if(pt != null && req.poiTypeFilter.accept(pt.name, subType.toString())) {
            //                        codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
            //                        return true;
            //                    }
            //                }
            //                break;
            default:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
        }
    }
}

bool OsmAnd::ObfPoiSectionReader_P::readAmenitiesDataBox(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const QString& query,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter,
    const ZoomLevel zoomFilter,
    const std::shared_ptr<QSet<uint64_t>> pTilesToSkip,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();

    auto zoom = InvalidZoomLevel;
    auto tileId = TileId::zero();
    bool firstAmenityRead = false;

    bool atLeastOneAccepted = false;

    for (;;)
    {
        const auto tag = cis->ReadTag();
        switch (gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
            case 0:
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return false;

                return atLeastOneAccepted;
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

                    if (!rejectBox && tileFilter)
                        rejectBox = !tileFilter(tileId, zoom);

                    if (!rejectBox && bbox31)
                    {
                        const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
                        rejectBox =
                            !bbox31->contains(tileBBox31) &&
                            !tileBBox31.contains(*bbox31) &&
                            !bbox31->intersects(tileBBox31);
                    }

                    if (rejectBox)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return false;
                    }
                }

                gpb::uint32 length;
                cis->ReadVarint32(&length);
                const auto offset = cis->CurrentPosition();
                const auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<const Amenity> amenity;
                readAmenity(
                    reader, 
                    section,
                    amenity,
                    query,
                    tileId,
                    zoom,
                    bbox31,
                    categoriesFilter,
                    queryController);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);

                if (!amenity)
                    break;

                auto tileValue = std::numeric_limits<uint64_t>::max();
                if (pTilesToSkip)
                {
                    TileId amenityTileId;
                    amenityTileId.x = amenity->position31.x >> (MaxZoomLevel - zoomFilter);
                    amenityTileId.y = amenity->position31.y >> (MaxZoomLevel - zoomFilter);
                    tileValue =
                        (static_cast<uint64_t>(amenityTileId.x) << zoomFilter) |
                        static_cast<uint64_t>(amenityTileId.y);

                    if (pTilesToSkip->contains(tileValue))
                    {
                        if (zoomFilter <= zoom)
                        {
                            cis->Skip(cis->BytesUntilLimit());
                            return atLeastOneAccepted;
                        }

                        break;
                    }
                }

                if (!visitor || visitor(amenity))
                {
                    if (outAmenities)
                        outAmenities->push_back(qMove(amenity));

                    if (pTilesToSkip)
                        pTilesToSkip->insert(tileValue);

                    atLeastOneAccepted = true;
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
    const TileId boxTileId,
    const ZoomLevel boxZoom,
    const AreaI* const bbox31,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    const auto baseOffset = cis->CurrentPosition();

    const auto subtypes = section->_p->_subtypes;

    ObfObjectId id;
    bool autogenerateId = true;
    std::shared_ptr<Amenity> amenity;
    PointI position31;
    QString nativeName;
    QHash<QString, QString> localizedNames;
    QList<ObfPoiCategoryId> categories;
    QVector<int> textValueSubtypeIndices;
    QHash<int, QVariant> intValues;
    QHash<int, QVariant> stringOrDataValues;
    auto categoriesFilterChecked = false;
    const CollatorStringMatcher matcher(query, StringMatcherMode::CHECK_STARTS_FROM_SPACE);
    uint32_t precisionXY = 0;

    for (;;)
    {
        const auto t = cis->ReadTag();
        const auto tag = gpb::internal::WireFormatLite::GetTagFieldNumber(t);
        if (categories.size() == 0 && (tag > OBF::OsmAndPoiBoxDataAtom::kCategoriesFieldNumber || tag == 0))
        {
            cis->Skip(cis->BytesUntilLimit());
            return;
        }
        switch (tag)
        {
            case 0:
            {
                if (!ObfReaderUtilities::reachedDataEnd(cis))
                    return;

                QList<QString> additionalNames;

                auto itStringOrDataValue = mutableIteratorOf(stringOrDataValues);
                while (itStringOrDataValue.hasNext())
                {
                    const auto& entry = itStringOrDataValue.next();

                    const auto& tag = subtypes->subtypes[entry.key()]->tagName;
                    if (tag.contains(QLatin1String("_name")))
                    {
                        additionalNames.append(entry.value().toString());
                    }
                    else if (tag == QLatin1String("name"))
                    {
                        nativeName = entry.value().toString();
                        itStringOrDataValue.remove();
                    }
                    else if (tag.startsWith(QLatin1String("name:")))
                    {
                        localizedNames.insert(tag.mid(5), entry.value().toString());
                        itStringOrDataValue.remove();
                    }
                    else if (tag == QLatin1String("brand"))
                    {
                        localizedNames.insert(tag, entry.value().toString());
                        continue;
                    }
                }

                if (!query.isNull())
                {
                    bool accept = !nativeName.isEmpty() &&
                    (matcher.matches(nativeName) || matcher.matches(OsmAnd::ICU::transliterateToLatin(nativeName)));
                    for (const auto& localizedName : constOf(localizedNames))
                    {
                        accept = accept || matcher.matches(localizedName);

                        if (accept)
                            break;
                    }
                    if (!accept)
                    {
                        for (const auto& additionalName : additionalNames)
                        {
                            accept = accept || matcher.matches(additionalName);
                            
                            if (accept)
                                break;
                        }
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
                if (precisionXY > 0)
                {
                    int xBase = position31.x >> BASE_POI_SHIFT;
                    int yBase = position31.y >> BASE_POI_SHIFT;
                    std::pair<int, int> precisedXY = OsmAnd::Utilities::calculateFinalXYFromBaseAndPrecisionXY(BASE_POI_ZOOM, FINAL_POI_ZOOM, precisionXY, xBase, yBase, true);
                    position31.x = precisedXY.first << FINAL_POI_SHIFT;
                    position31.y = precisedXY.second << FINAL_POI_SHIFT;
                }
                amenity->position31 = position31;
                amenity->categories = qMove(categories);
                if (autogenerateId)
                    amenity->id = ObfObjectId::generateUniqueId(baseOffset, section);
                else
                    amenity->id = id;
                amenity->values = detachedOf(intValues).unite(stringOrDataValues);
                amenity->evaluateTypes();
                outAmenity = amenity;

                //////////////////////////////////////////////////////////////////////////
                //if (amenity->id.getOsmId() == 582502308u)
                //{
                //    int i = 5;
                //}
                //////////////////////////////////////////////////////////////////////////

                return;
            }
            case OBF::OsmAndPoiBoxDataAtom::kDxFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                assert(d >= 0);
                position31.x = ((boxTileId.x << (BASE_POI_ZOOM - boxZoom)) + d) << BASE_POI_SHIFT;
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kDyFieldNumber:
            {
                const auto d = ObfReaderUtilities::readSInt32(cis);
                assert(d >= 0);
                position31.y = ((boxTileId.y << (BASE_POI_ZOOM - boxZoom)) + d) << BASE_POI_SHIFT;

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
                    ? (rawValue >> 16)
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
                if (textValueSubtypeIndices.size() == 0)
                    break;

                const auto subtypeIndex = textValueSubtypeIndices[0];
                textValueSubtypeIndices.pop_front();
                if (valueString.startsWith(QLatin1String(" gz ")) && valueString.size() >= 4)
                {
                    const auto dataSize = valueString.size() - 4;
                    QByteArray data(dataSize, Qt::Initialization::Uninitialized);

                    auto pSrc = valueString.data() + 4;
                    auto pDst = reinterpret_cast<uint8_t*>(data.data());
                    for (auto idx = 0u; idx < dataSize; idx++)
                    {
                        const auto src = *pSrc++;
                        const auto dst = src.unicode() - 128 - 32;
                        *pDst++ = static_cast<uint8_t>(dst);
                    }

                    stringOrDataValues.insert(subtypeIndex, QVariant(data));
                }
                else
                    stringOrDataValues.insert(subtypeIndex, valueString);

                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kOpeningHoursFieldNumber:
            {
                QString valueString;
                ObfReaderUtilities::readQString(cis, valueString);

                stringOrDataValues.insert(subtypes->openingHoursSubtypeIndex, valueString);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kSiteFieldNumber:
            {
                QString valueString;
                ObfReaderUtilities::readQString(cis, valueString);

                stringOrDataValues.insert(subtypes->websiteSubtypeIndex, valueString);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kPhoneFieldNumber:
            {
                QString valueString;
                ObfReaderUtilities::readQString(cis, valueString);

                stringOrDataValues.insert(subtypes->phoneSubtypeIndex, valueString);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kNoteFieldNumber:
            {
                QString valueString;
                ObfReaderUtilities::readQString(cis, valueString);

                stringOrDataValues.insert(subtypes->descriptionSubtypeIndex, valueString);
                break;
            }
            case OBF::OsmAndPoiBoxDataAtom::kPrecisionXYFieldNumber:
            {
                cis->ReadVarint32(&precisionXY);
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
    const PointI* const xy31,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    const auto cis = reader.getCodedInputStream().get();
    QMap<uint32_t, uint32_t> dataBoxesOffsetsSet;
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
                    dataBoxesOffsetsSet,
                    xy31,
                    bbox31,
                    tileFilter);

                ObfReaderUtilities::ensureAllDataWasRead(cis);
                cis->PopLimit(oldLimit);
                break;
            }
            case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
                ObfReaderUtilities::skipUnknownField(cis, tag);
                break;
            case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            {
                auto offKeys = dataBoxesOffsetsSet.keys();
                std::sort(offKeys,
                          [&dataBoxesOffsetsSet]
                          (const uint32_t& offset1, const uint32_t& offset2) -> bool
                          {
                              return dataBoxesOffsetsSet[offset1] < dataBoxesOffsetsSet[offset2];
                          });
                
                int p = BUCKET_SEARCH_BY_NAME * 3;
                if (p < offKeys.length())
                {
                    for (int i = p + BUCKET_SEARCH_BY_NAME; ; i += BUCKET_SEARCH_BY_NAME)
                    {
                        if (i > offKeys.length())
                        {
                            std::sort(offKeys.begin() + p, offKeys.end());
                            break;
                        }
                        else
                        {
                            std::sort(offKeys.begin() + p, offKeys.begin() + i);
                        }
                        p = i;
                    }
                }
                
                for (const auto dataOffset : offKeys)
                {
                    cis->Seek(section->offset + dataOffset);
                    const auto length = ObfReaderUtilities::readBigEndianInt(cis);
                    const auto offset = cis->CurrentPosition();
                    const auto oldLimit = cis->PushLimit(length);

                    readAmenitiesDataBox(
                        reader,
                        section,
                        outAmenities,
                        query,
                        bbox31,
                        tileFilter,
                        InvalidZoomLevel,
                        nullptr,
                        categoriesFilter,
                        visitor,
                        queryController);

                    ObfReaderUtilities::ensureAllDataWasRead(cis);
                    cis->PopLimit(oldLimit);
                    if (queryController && queryController->isAborted())
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
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
    QMap<uint32_t, uint32_t>& outDataOffsets,
    const PointI* const xy31,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter)
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

                    readNameIndexData(
                        reader,
                        outDataOffsets,
                        xy31,
                        bbox31, 
                        tileFilter);
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
    QMap<uint32_t, uint32_t>& outDataOffsets,
    const PointI* const xy31,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter)
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

                readNameIndexDataAtom(
                    reader,
                    outDataOffsets,
                    xy31,
                    bbox31,
                    tileFilter);
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
    QMap<uint32_t, uint32_t>& outDataOffsets,
    const PointI* const xy31,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter)
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

                bool accept = true;

                if (accept && tileFilter)
                    accept = tileFilter(tileId, zoom);

                PointI position31;
                uint32_t d = 0;
                if (accept && bbox31)
                {
                    position31.x = tileId.x << (31 - zoom);
                    position31.y = tileId.y << (31 - zoom);
                    accept = bbox31->contains(position31);
                }

                if (accept)
                {
                    if (xy31)
                        d = qAbs(xy31->x - position31.x) + qAbs(xy31->y - position31.y);
                    outDataOffsets.insert(dataOffset, d);
                }
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
    const std::shared_ptr<const IQueryController>& queryController)
{
    ensureCategoriesLoaded(reader, section);
    outCategories = section->getCategories();
}

void OsmAnd::ObfPoiSectionReader_P::loadSubtypes(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
    const std::shared_ptr<const IQueryController>& queryController)
{
    ensureSubtypesLoaded(reader, section);
    outSubtypes = section->getSubtypes();
}

void OsmAnd::ObfPoiSectionReader_P::loadAmenities(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter,
    const ZoomLevel zoomFilter,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    ensureCategoriesLoaded(reader, section);
    ensureSubtypesLoaded(reader, section);

    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    auto oldLimit = cis->PushLimit(section->length);
    cis->Skip(section->firstBoxInnerOffset);

    readAmenities(
        reader,
        section,
        outAmenities,
        bbox31,
        tileFilter,
        zoomFilter,
        categoriesFilter,
        visitor,
        queryController);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSectionReader_P::scanAmenitiesByName(
    const ObfReader_P& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    const QString& query,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const PointI* const xy31,
    const AreaI* const bbox31,
    const TileAcceptorFunction tileFilter,
    const QSet<ObfPoiCategoryId>* const categoriesFilter,
    const ObfPoiSectionReader::VisitorFunction visitor,
    const std::shared_ptr<const IQueryController>& queryController)
{
    ensureCategoriesLoaded(reader, section);
    ensureSubtypesLoaded(reader, section);

    const auto cis = reader.getCodedInputStream().get();
    cis->Seek(section->offset);
    auto oldLimit = cis->PushLimit(section->length);
    cis->Skip(section->nameIndexInnerOffset);

    readAmenitiesByName(
        reader,
        section,
        query,
        outAmenities,
        xy31,
        bbox31,
        tileFilter,
        categoriesFilter,
        visitor,
        queryController);

    ObfReaderUtilities::ensureAllDataWasRead(cis);
    cis->PopLimit(oldLimit);
}
