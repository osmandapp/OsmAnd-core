#include "ObfPoiSectionReader_P.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include "ObfReader_P.h"
#include "ObfPoiSectionInfo.h"
#include "Amenity.h"
#include "AmenityCategory.h"
#include "ObfReaderUtilities.h"
#include "IQueryController.h"
#include "ICU.h"
#include "Utilities.h"

OsmAnd::ObfPoiSectionReader_P::ObfPoiSectionReader_P()
{
}

OsmAnd::ObfPoiSectionReader_P::~ObfPoiSectionReader_P()
{
}

void OsmAnd::ObfPoiSectionReader_P::read( const ObfReader_P& reader, const std::shared_ptr<ObfPoiSectionInfo>& section )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, section->_name);
            break;
        case OBF::OsmAndPoiIndex::kBoundariesFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                readBoundaries(reader, section);
                cis->PopLimit(oldLimit);
            }
            break; 
        case OBF::OsmAndPoiIndex::kCategoriesTableFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readBoundaries( const ObfReader_P& reader, const std::shared_ptr<ObfPoiSectionInfo>& section )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndTileBox::kLeftFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_area31.left()));
            break;
        case OBF::OsmAndTileBox::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_area31.right()));
            break;
        case OBF::OsmAndTileBox::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_area31.top()));
            break;
        case OBF::OsmAndTileBox::kBottomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gpb::uint32*>(&section->_area31.bottom()));
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readCategories(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const Model::AmenityCategory> >& categories )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kCategoriesTableFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                
                std::shared_ptr<Model::AmenityCategory> category(new Model::AmenityCategory());
                readCategory(reader, category);
                
                cis->PopLimit(oldLimit);

                categories.push_back(qMove(category));
            }
            break;
        case OBF::OsmAndPoiIndex::kNameIndexFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readCategory( const ObfReader_P& reader, const std::shared_ptr<Model::AmenityCategory>& category )
{
    auto cis = reader._codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndCategoryTable::kCategoryFieldNumber:
            ObfReaderUtilities::readQString(cis, category->_name);
            break;
        case OBF::OsmAndCategoryTable::kSubcategoriesFieldNumber:
            {
                QString name;
                if (ObfReaderUtilities::readQString(cis, name))
                    category->_subcategories.push_back(qMove(name));
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::loadCategories(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Model::AmenityCategory> >& categories )
{
    auto cis = reader._codedInputStream.get();
    cis->Seek(section->_offset);
    auto oldLimit = cis->PushLimit(section->_length);
    readCategories(reader, section, categories);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSectionReader_P::loadAmenities(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    const ZoomLevel zoom, uint32_t zoomDepth /*= 3*/, const AreaI* bbox31 /*= nullptr*/,
    QSet<uint32_t>* desiredCategories /*= nullptr*/,
    QList< Model::Amenity::RefC >* amenitiesOut /*= nullptr*/,
    std::function<bool (Model::Amenity::NCRefC)> visitor /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/ )
{
    auto cis = reader._codedInputStream.get();
    cis->Seek(section->_offset);
    auto oldLimit = cis->PushLimit(section->_length);
    readAmenities(reader, section, desiredCategories, amenitiesOut, zoom, zoomDepth, bbox31, visitor, controller);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSectionReader_P::readAmenities(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QSet<uint32_t>* desiredCategories,
    QList< Model::Amenity::RefC >* amenitiesOut,
    const ZoomLevel zoom, uint32_t zoomDepth, const AreaI* bbox31,
    std::function<bool (Model::Amenity::NCRefC)> visitor,
    const IQueryController* const controller)
{
    auto cis = reader._codedInputStream.get();
    QList< std::shared_ptr<Tile> > tiles;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            {
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto oldLimit = cis->PushLimit(length);
                readTile(reader, section, tiles, nullptr, desiredCategories, zoom, zoomDepth, bbox31, controller, nullptr);
                cis->PopLimit(oldLimit);
                if (controller && controller->isAborted())
                    return;
            }
            break;
        case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            {
                // Sort tiles byte data offset, to all cache-friendly with I/O system
                qSort(tiles.begin(), tiles.end(), [](const std::shared_ptr<Tile>& l, const std::shared_ptr<Tile>& r) -> bool
                {
                    return l->_hash < r->_hash;
                });

                for(const auto& tile : constOf(tiles))
                {
                    cis->Seek(section->_offset + tile->_offset);
                    auto length = ObfReaderUtilities::readBigEndianInt(cis);
                    auto oldLimit = cis->PushLimit(length);
                    readAmenitiesFromTile(reader, section, tile.get(), desiredCategories, amenitiesOut, zoom, zoomDepth, bbox31, visitor, controller, nullptr);
                    cis->PopLimit(oldLimit);
                    if (controller && controller->isAborted())
                        return;
                }
                cis->Skip(cis->BytesUntilLimit());
            }
            return;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

bool OsmAnd::ObfPoiSectionReader_P::readTile(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<Tile> >& tiles,
    Tile* parent,
    QSet<uint32_t>* desiredCategories,
    uint32_t zoom, uint32_t zoomDepth, const AreaI* bbox31,
    const IQueryController* const controller,
    QSet< uint64_t >* tilesToSkip)
{
    auto cis = reader._codedInputStream.get();

    const auto zoomToSkip = zoom + zoomDepth;
    QSet< uint64_t > tilesToSkip_;
    if (parent == nullptr && !tilesToSkip)
        tilesToSkip = &tilesToSkip_;

    const std::shared_ptr<Tile> tile(new Tile());
    gpb::uint32 lzoom;

    for(;;)
    {
        if (controller && controller->isAborted())
            return false;
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            tiles.push_back(qMove(tile));
            return true;
        case OBF::OsmAndPoiBox::kZoomFieldNumber:
            {
                cis->ReadVarint32(&lzoom);
                tile->_zoom = lzoom;
                if (parent)
                    tile->_zoom += parent->_zoom;
            }
            break;
        case OBF::OsmAndPoiBox::kLeftFieldNumber:
            {
                auto x = ObfReaderUtilities::readSInt32(cis);
                if (parent)
                    tile->_x = x + (parent->_x << lzoom);
                else
                    tile->_x = x;
            }
            break;
        case OBF::OsmAndPoiBox::kTopFieldNumber:
            {
                auto y = ObfReaderUtilities::readSInt32(cis);
                if (parent)
                    tile->_y = y + (parent->_y << lzoom);
                else
                    tile->_y = y;

                // Check that we're inside bounding box, if requested
                if (bbox31)
                {
                    AreaI area31;
                    area31.left() = tile->_x << (31 - tile->_zoom);
                    area31.right() = (tile->_x + 1) << (31 - tile->_zoom);
                    area31.top() = tile->_y << (31 - tile->_zoom);
                    area31.bottom() = (tile->_y + 1) << (31 - tile->_zoom);

                    const auto shouldSkip =
                        !bbox31->contains(area31) &&
                        !area31.contains(*bbox31) &&
                        !bbox31->intersects(area31);
                    if (shouldSkip)
                    {
                        // This tile is outside of bounding box
                        cis->Skip(cis->BytesUntilLimit());
                        return false;
                    }
                }
            }
            break;
        case OBF::OsmAndPoiBox::kCategoriesFieldNumber:
            {
                if (!desiredCategories)
                {
                    ObfReaderUtilities::skipUnknownField(cis, tag);
                    break;
                }
                gpb::uint32 length;
                cis->ReadLittleEndian32(&length);
                auto oldLimit = cis->PushLimit(length);
                const auto containsDesired = checkTileCategories(reader, section, desiredCategories);
                cis->PopLimit(oldLimit);
                if (!containsDesired)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return false;
                }
            }
            break;
        case OBF::OsmAndPoiBox::kSubBoxesFieldNumber:
            {
                auto length = ObfReaderUtilities::readBigEndianInt(cis);
                auto oldLimit = cis->PushLimit(length);
                auto tileOmitted = readTile(reader, section, tiles, tile.get(), desiredCategories, zoom, zoomDepth, bbox31, controller, tilesToSkip);
                cis->PopLimit(oldLimit);

                if (tilesToSkip && tile->_zoom >= zoomToSkip && tileOmitted)
                {
                    auto skipHash = (static_cast<uint64_t>(tile->_x) >> (tile->_zoom - zoomToSkip)) << zoomToSkip;
                    skipHash |= static_cast<uint64_t>(tile->_y) >> (tile->_zoom - zoomToSkip);
                    if (tilesToSkip->contains(skipHash))
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return true;
                    }
                }
            }
            break;
        case OBF::OsmAndPoiBox::kShiftToDataFieldNumber:
            {
                tile->_offset = ObfReaderUtilities::readBigEndianInt(cis);
                tile->_hash  = static_cast<uint64_t>(tile->_x) << tile->_zoom;
                tile->_hash |= static_cast<uint64_t>(tile->_y);
                tile->_hash |= tile->_zoom;

                // skipTiles - these tiles are going to be ignored, since we need only 1 POI object (x;y)@zoom
                if (tilesToSkip && tile->_zoom >= zoomToSkip)
                {
                    auto skipHash = (static_cast<uint64_t>(tile->_x) >> (tile->_zoom - zoomToSkip)) << zoomToSkip;
                    skipHash |= static_cast<uint64_t>(tile->_y) >> (tile->_zoom - zoomToSkip);
                    tilesToSkip->insert(skipHash);
                }
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

bool OsmAnd::ObfPoiSectionReader_P::checkTileCategories(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QSet<uint32_t>* desiredCategories )
{
    auto cis = reader._codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return false;
        case OBF::OsmAndPoiCategories::kCategoriesFieldNumber:
            {
                gpb::uint32 binaryMixedId;
                cis->ReadVarint32(&binaryMixedId);
                const auto catId = binaryMixedId & CategoryIdMask;
                const auto subId = binaryMixedId >> SubcategoryIdShift;

                const uint32_t allSubsId = (catId << 16) | 0xFFFF;
                const uint32_t mixedId = (catId << 16) | subId;
                if (desiredCategories->contains(allSubsId) || desiredCategories->contains(mixedId))
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return true;
                }
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readAmenitiesFromTile(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section, Tile* tile,
    QSet<uint32_t>* desiredCategories,
    QList< Model::Amenity::RefC >* amenitiesOut,
    const ZoomLevel zoom, uint32_t zoomDepth, const AreaI* bbox31,
    std::function<bool (Model::Amenity::NCRefC)> visitor,
    const IQueryController* const controller,
    QSet< uint64_t >* amenitiesToSkip)
{
    auto cis = reader._codedInputStream.get();

    const auto zoomToSkip = zoom + zoomDepth;
    QSet< uint64_t > amenitiesToSkip_;
    if (!amenitiesToSkip)
        amenitiesToSkip = &amenitiesToSkip_;

    PointI pTile;
    uint32_t zoomTile = 0;

    for(;;)
    {
        if (controller && controller->isAborted())
            return;

        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiBoxData::kZoomFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                zoomTile = value;
            }
            break;
        case OBF::OsmAndPoiBoxData::kXFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                pTile.x = value;
            }
            break;
        case OBF::OsmAndPoiBoxData::kYFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                pTile.y = value;
            }
            break;
        case OBF::OsmAndPoiBoxData::kPoiDataFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                Model::Amenity::RefC amenity;
                readAmenity(reader, section, pTile, zoomTile, amenity, desiredCategories, bbox31, controller);

                cis->PopLimit(oldLimit);

                if (!amenity)
                    break;
                if (amenitiesToSkip)
                {
                    const auto xp = amenity->_point31.x >> (31 - zoomToSkip);
                    const auto yp = amenity->_point31.y >> (31 - zoomToSkip);
                    const auto hash = (static_cast<uint64_t>(xp) << zoomToSkip) | static_cast<uint64_t>(yp);
                    if (!amenitiesToSkip->contains(hash))
                    {
                        if (!visitor || visitor(amenity))
                        {
                            amenitiesToSkip->insert(hash);
                            if (amenitiesOut)
                                amenitiesOut->push_back(qMove(amenity));
                        }
                    }
                    if (zoomToSkip <= zoom)
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                else
                {
                    if (amenitiesOut && (!visitor || visitor(amenity)))
                        amenitiesOut->push_back(qMove(amenity));
                }
            }
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSectionReader_P::readAmenity(
    const ObfReader_P& reader, const std::shared_ptr<const ObfPoiSectionInfo>& section,
    const PointI& pTile, uint32_t pzoom,
    Model::Amenity::OutRefC outAmenity,
    QSet<uint32_t>* desiredCategories,
    const AreaI* bbox31,
    const IQueryController* const controller)
{
    auto cis = reader._codedInputStream.get();
    PointI point;
    uint32_t catId;
    uint32_t subId;
    Model::Amenity::Ref amenity;
    for(;;)
    {
        if (controller && controller->isAborted())
            return;

        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if (amenity->_latinName.isEmpty())
                amenity->_latinName = ICU::transliterateToLatin(amenity->_name);
            amenity->_point31 = point;
            amenity->_categoryId = catId;
            amenity->_subcategoryId = subId;
            outAmenity = amenity;
            return;
        case OBF::OsmAndPoiBoxDataAtom::kDxFieldNumber:
            point.x = (ObfReaderUtilities::readSInt32(cis) + (pTile.x << (24 - pzoom))) << 7;
            break;
        case OBF::OsmAndPoiBoxDataAtom::kDyFieldNumber:
            point.y = (ObfReaderUtilities::readSInt32(cis) + (pTile.y << (24 - pzoom))) << 7;
            if (bbox31 && !bbox31->contains(point))
            {
                cis->Skip(cis->BytesUntilLimit());
                return;
            }
            break;
        case OBF::OsmAndPoiBoxDataAtom::kCategoriesFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                catId = value & CategoryIdMask;
                subId = value >> SubcategoryIdShift;

                if (desiredCategories)
                {
                    const uint32_t allSubsId = (catId << 16) | 0xFFFF;
                    const uint32_t mixedId = (catId << 16) | subId;
                    if (desiredCategories->contains(allSubsId) || desiredCategories->contains(mixedId))
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                amenity = (new Model::Amenity())->getRef();
            }
            break;
        case OBF::OsmAndPoiBoxDataAtom::kIdFieldNumber:
            {
                cis->ReadVarint64(reinterpret_cast<gpb::uint64*>(&amenity->_id));
            }
            break;
        case OBF::OsmAndPoiBoxDataAtom::kNameFieldNumber:
            ObfReaderUtilities::readQString(cis, amenity->_name);
            break;
        case OBF::OsmAndPoiBoxDataAtom::kNameEnFieldNumber:
            ObfReaderUtilities::readQString(cis, amenity->_latinName);
            break;
        case OBF::OsmAndPoiBoxDataAtom::kOpeningHoursFieldNumber:
            ObfReaderUtilities::readQString(cis, amenity->_openingHours);
            break;
        case OBF::OsmAndPoiBoxDataAtom::kSiteFieldNumber:
            ObfReaderUtilities::readQString(cis, amenity->_site);
            break;
        case OBF::OsmAndPoiBoxDataAtom::kPhoneFieldNumber:
            ObfReaderUtilities::readQString(cis, amenity->_phone);
            break;
        case OBF::OsmAndPoiBoxDataAtom::kNoteFieldNumber:
            ObfReaderUtilities::readQString(cis, amenity->_description);
            break;
        default:
            ObfReaderUtilities::skipUnknownField(cis, tag);
            break;
        }
    }
}