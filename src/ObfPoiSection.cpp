#include "ObfPoiSection.h"

#include <ObfReader.h>
#include <google/protobuf/wire_format_lite.h>

#include "Utilities.h"
#include "OBF.pb.h"

OsmAnd::ObfPoiSection::ObfPoiSection( class ObfReader* owner )
    : ObfSection(owner)
{
}

OsmAnd::ObfPoiSection::~ObfPoiSection()
{
}

void OsmAnd::ObfPoiSection::read( ObfReader* reader, ObfPoiSection* section)
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                section->_name = QString::fromStdString(name);
            }
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
        case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readCategories( ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<OsmAnd::Model::Amenity::Category> >& categories )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kNameFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                section->_name = QString::fromStdString(name);
            }
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
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<Model::Amenity::Category> category(new Model::Amenity::Category());
                readCategory(reader, category.get());
                cis->PopLimit(oldLimit);
                categories.push_back(category);
            }
            break;
        case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readBoundaries( ObfReader* reader, ObfPoiSection* section )
{
    auto cis = reader->_codedInputStream.get();

    gpb::uint32 value;
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndTileBox::kLeftFieldNumber:
            cis->ReadVarint32(&value);
            section->_leftLongitude = Utilities::get31LongitudeX(value);
            break;
        case OBF::OsmAndTileBox::kRightFieldNumber:
            cis->ReadVarint32(&value);
            section->_rightLongitude = Utilities::get31LongitudeX(value);
            break;
        case OBF::OsmAndTileBox::kTopFieldNumber:
            cis->ReadVarint32(&value);
            section->_topLatitude = Utilities::get31LatitudeY(value);
            break;
        case OBF::OsmAndTileBox::kBottomFieldNumber:
            cis->ReadVarint32(&value);
            section->_bottomLatitude = Utilities::get31LatitudeY(value);
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readCategory( ObfReader* reader, OsmAnd::Model::Amenity::Category* category )
{
    auto cis = reader->_codedInputStream.get();

    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndCategoryTable::kCategoryFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                category->_name = QString::fromStdString(name);
                //TODO:region.categoriesType.add(AmenityType.fromString(cat));
            }
            break;
        case OBF::OsmAndCategoryTable::kSubcategoriesFieldNumber:
            {
                std::string name;
                gpb::internal::WireFormatLite::ReadString(cis, &name);
                category->_subcategories.push_back(QString::fromStdString(name));
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::loadCategories( OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, QList< std::shared_ptr<OsmAnd::Model::Amenity::Category> >& categories )
{
    auto cis = reader->_codedInputStream.get();
    cis->Seek(section->_offset);
    auto oldLimit = cis->PushLimit(section->_length);
    readCategories(reader, section, categories);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSection::loadAmenities( OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, QList< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities )
{
    auto cis = reader->_codedInputStream.get();
    cis->Seek(section->_offset);
    auto oldLimit = cis->PushLimit(section->_length);
    readAmenities(reader, section, amenities);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSection::readAmenities( ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities )
{
    auto cis = reader->_codedInputStream.get();

    QList< std::shared_ptr<Tile> > tiles;
    readTiles(reader, section, tiles);
    if(tiles.count() == 0)
        return;

    // Sort tiles byte data offset, to all cache-friendly with I/O system
    qSort(tiles.begin(), tiles.end(), [](const std::shared_ptr<Tile>& l, const std::shared_ptr<Tile>& r) -> int
    {
        return l->_hash - r->_hash;
    });

    readAmenitiesFromTiles(reader, section, tiles, amenities);
}

void OsmAnd::ObfPoiSection::readTiles( ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Tile> >& tiles )
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto oldLimit = cis->PushLimit(length);
                readTile(reader, section, tiles, nullptr);
                cis->PopLimit(oldLimit);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readTile( ObfReader* reader, ObfPoiSection* section, QList< std::shared_ptr<Tile> >& tiles, Tile* parent )
{
    auto cis = reader->_codedInputStream.get();

    /*req.numberOfReadSubtrees++;
    int zoomToSkip = req.zoom + ZOOM_TO_SKIP_FILTER;
    boolean checkBox = true;
    boolean existsCategories = false;
    */

    std::shared_ptr<Tile> tile(new Tile());
    gpb::uint32 lzoom;
    
    for(;;)
    {
        /*if(req.isCancelled()){
            return;
        }*/
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            tiles.push_back(tile);
            return;
        case OBF::OsmAndPoiBox::kZoomFieldNumber:
            {
                cis->ReadVarint32(&lzoom);
                tile->_zoom = lzoom;
                if(parent)
                    tile->_zoom += parent->_zoom;
            }
            break;
        case OBF::OsmAndPoiBox::kLeftFieldNumber:
            {
                tile->_x = ObfReader::readSInt32(cis);
                if(parent)
                    tile->_x += (parent->_x << lzoom);
            }
            break;
        case OBF::OsmAndPoiBox::kTopFieldNumber:
            {
                tile->_y = ObfReader::readSInt32(cis);
                if(parent)
                    tile->_y += (parent->_y << lzoom);
            }
            break;
        case OBF::OsmAndPoiBox::kCategoriesFieldNumber:
            {
                if(/*req.poiTypeFilter == null*/false)
                {
                    ObfReader::skipUnknownField(cis, tag);
                    break;
                }
                /*
                int length = codedIS.readRawVarint32();
                int oldLimit = codedIS.pushLimit(length);
                //TODO: check that CURRENT tile has needed categories, and exit if no
                boolean check = checkCategories(req, region);
                codedIS.popLimit(oldLimit);
                if(!check){
                    codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                    return false;
                }
                existsCategories = true;
            */
            }
            break;
        case OBF::OsmAndPoiBox::kSubBoxesFieldNumber:
            {
                /*
                if(checkBox){
                    int xL = x << (31 - zoom);
                    int xR = (x + 1) << (31 - zoom);
                    int yT = y << (31 - zoom);
                    int yB = (y + 1) << (31 - zoom);
                    // check intersection
                    if(left31 > xR || xL > right31 || bottom31 < yT || yB < top31){
                        codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                        return false;
                    }
                    req.numberOfAcceptedSubtrees++;
                    checkBox = false;
                }
                */
                auto length = ObfReader::readBigEndianInt(cis);
                auto oldLimit = cis->PushLimit(length);
                //boolean exists = readBoxField(left31, right31, top31, bottom31, x, y, zoom, offsetsMap, skipTiles, req, region);
                readTile(reader, section, tiles, tile.get());
                cis->PopLimit(oldLimit);

                /*
                if (skipTiles != null && zoom >= zoomToSkip && exists) {
                    long val = ((((long) x) >> (zoom - zoomToSkip)) << zoomToSkip) | (((long) y) >> (zoom - zoomToSkip));
                    if(skipTiles.contains(val)){
                        codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                        return true;
                    }
                }
                */
            }
            break;
        case OBF::OsmAndPoiBox::kShiftToDataFieldNumber:
            {
                tile->_hash  = static_cast<uint64_t>(tile->_x) << tile->_zoom;
                tile->_hash |= static_cast<uint64_t>(tile->_y);
                tile->_hash |= tile->_zoom;
                tile->_offset = ObfReader::readBigEndianInt(cis);
                // skipTiles - these tiles are going to be ignored, since we need only 1 POI object (x;y)@zoom
                /*
                offsetsMap.put(readInt(), l);
                if(skipTiles != null && zoom >= zoomToSkip){
                    long val = ((((long) x) >> (zoom - zoomToSkip)) << zoomToSkip) | (((long) y) >> (zoom - zoomToSkip));
                    skipTiles.add(val);
                }
                */
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readAmenitiesFromTiles( ObfReader* reader, ObfPoiSection* section, const QList< std::shared_ptr<Tile> >& tiles, QList< std::shared_ptr<Model::Amenity> >& amenities )
{
    auto cis = reader->_codedInputStream.get();
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            {
                for(auto itTile = tiles.begin(); itTile != tiles.end(); ++itTile)
                {
                    auto tile = *itTile;

                    cis->Seek(section->_offset + tile->_offset);
                    auto length = ObfReader::readBigEndianInt(cis);
                    auto oldLimit = cis->PushLimit(length);
                    readAmenitiesFromTile(reader, section, tile.get(), amenities);
                    cis->PopLimit(oldLimit);
                    /*if(req.isCancelled())
                        return;*/
                }
                cis->Skip(cis->BytesUntilLimit());
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readAmenitiesFromTile( ObfReader* reader, ObfPoiSection* section, Tile* tile, QList< std::shared_ptr<Model::Amenity> >& amenities )
{
    auto cis = reader->_codedInputStream.get();
    int32_t x = 0;
    int32_t y = 0;
    uint32_t zoom = 0;
    for(;;)
    {
        /*if(req.isCancelled()){
            return;
        }*/
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OBF::OsmAndPoiBoxData::kZoomFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                zoom = value;
            }
            break;
        case OBF::OsmAndPoiBoxData::kXFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                x = value;
            }
            break;
        case OBF::OsmAndPoiBoxData::kYFieldNumber:
            {
                gpb::uint32 value;
                cis->ReadVarint32(&value);
                y = value;
            }
            break;
        case OBF::OsmAndPoiBoxData::kPoiDataFieldNumber:
            {
                gpb::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);
                std::shared_ptr<Model::Amenity> amenity;
                readAmenity(reader, section, x, y, zoom, amenity);
                cis->PopLimit(oldLimit);
                /*
                if (am != null) {
                    if (toSkip != null) {
                        int xp = (int) MapUtils.getTileNumberX(zSkip, am.getLocation().getLongitude());
                        int yp = (int) MapUtils.getTileNumberY(zSkip, am.getLocation().getLatitude());
                        long val = (((long) xp) << zSkip) | yp;
                        if (!toSkip.contains(val)) {
                            boolean publish = req.publish(am);
                            if(publish) {
                                toSkip.add(val);
                            }
                        }
                        if(zSkip <= zoom){
                            codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                            return;
                        }
                    } else {
                        req.publish(am);
                    }
                }*/
                if(amenity)
                    amenities.push_back(amenity);
            }
            break;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readAmenity( ObfReader* reader, ObfPoiSection* section, int32_t px, int32_t py, uint32_t pzoom, std::shared_ptr<Model::Amenity>& amenity )
{
    auto cis = reader->_codedInputStream.get();
    int x;
    int y;
    //AmenityType amenityType = null;
    for(;;)
    {
        /*if(req.isCancelled()){
            return;
        }*/
        auto tag = cis->ReadTag();
        switch(gpb::internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(amenity->_latinName.isEmpty())
                amenity->_latinName = section->_owner->transliterate(amenity->_name);
            return;
        case OBF::OsmAndPoiBoxDataAtom::kDxFieldNumber:
            x = (ObfReader::readSInt32(cis) + (px << (24 - pzoom))) << 7;
            break;
        case OBF::OsmAndPoiBoxDataAtom::kDyFieldNumber:
            y = (ObfReader::readSInt32(cis) + (py << (24 - pzoom))) << 7;
            /*req.numberOfVisitedObjects++;
            if (checkBounds) {
                if (left31 > x || right31 < x || top31 > y || bottom31 < y) {
                    codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                    return null;
                }
            }*/
            amenity.reset(new Model::Amenity());
            //am.setLocation(MapUtils.get31LatitudeY(y), MapUtils.get31LongitudeX(x));
            break;
        case OBF::OsmAndPoiBoxDataAtom::kCategoriesFieldNumber:
            /*int cat = codedIS.readUInt32();
            int subcatId = cat >> SHIFT_BITS_CATEGORY;
            int catId = cat & CATEGORY_MASK;
            AmenityType type = AmenityType.OTHER;
            String subtype = "";
            if (catId < region.categoriesType.size()) {
                type = region.categoriesType.get(catId);
                List<String> subcats = region.subcategories.get(catId);
                if (subcatId < subcats.size()) {
                    subtype = subcats.get(subcatId);
                }
            }
            if (req.poiTypeFilter == null || req.poiTypeFilter.accept(type, subtype)) {
                if (amenityType == null) {
                    amenityType = type;
                    am.setSubType(subtype);
                    am.setType(amenityType);
                } else {
                    am.setSubType(am.getSubType() + ";" + subtype);
                }
            }
            */
            break;
        /*case OBF::OsmAndPoiBoxDataAtom::kIdFieldNumber:
            am.setId(codedIS.readUInt64());
            break;
        case OBF::OsmAndPoiBoxDataAtom::kNameFieldNumber:
            am.setName(codedIS.readString());
            break;
        case OBF::OsmAndPoiBoxDataAtom::kNameEnFieldNumber:
            am.setEnName(codedIS.readString());
            break;
        case OBF::OsmAndPoiBoxDataAtom::kOpeningHoursFieldNumber:
            am.setOpeningHours(codedIS.readString());
            break;
        case OBF::OsmAndPoiBoxDataAtom::kSiteFieldNumber:
            am.setSite(codedIS.readString());
            break;
        case OBF::OsmAndPoiBoxDataAtom::kPhoneFieldNumber:
            am.setPhone(codedIS.readString());
            break;
        case OBF::OsmAndPoiBoxDataAtom::kNoteFieldNumber:
            am.setDescription(codedIS.readString());
            break;*/
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}
