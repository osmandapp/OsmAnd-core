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
                readPoiBoundariesIndex(reader, section);
                cis->PopLimit(oldLimit);
            }
            break; 
        case OBF::OsmAndPoiIndex::kCategoriesTableFieldNumber:
            cis->Skip(cis->BytesUntilLimit());
            return;
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

void OsmAnd::ObfPoiSection::readCategories( ObfReader* reader, ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::Model::Amenity::Category> >& categories )
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
                readPoiBoundariesIndex(reader, section);
                cis->PopLimit(oldLimit);
            }
            break; 
        case OBF::OsmAndPoiIndex::kCategoriesTableFieldNumber:
            {
                if(!readCategories)
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return;
                }
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

void OsmAnd::ObfPoiSection::readPoiBoundariesIndex( ObfReader* reader, ObfPoiSection* section )
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

void OsmAnd::ObfPoiSection::loadCategories( OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::Model::Amenity::Category> >& categories )
{
    auto cis = reader->_codedInputStream.get();
    cis->Seek(section->_offset);
    auto oldLimit = cis->PushLimit(section->_length);
    readCategories(reader, section, categories);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSection::loadAmenities( OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities )
{
    auto cis = reader->_codedInputStream.get();
    cis->Seek(section->_offset);
    auto oldLimit = cis->PushLimit(section->_length);
    readAmenities(reader, section, amenities);
    cis->PopLimit(oldLimit);
}

void OsmAnd::ObfPoiSection::readAmenities( ObfReader* reader, ObfPoiSection* section, std::list< std::shared_ptr<OsmAnd::Model::Amenity> >& amenities )
{
    auto cis = reader->_codedInputStream.get();

    /*int indexOffset = codedIS.getTotalBytesRead();
    long time = System.currentTimeMillis();
    TLongHashSet skipTiles = null;
    int zoomToSkip = 31;
    if(req.zoom != -1){
        skipTiles = new TLongHashSet();
        zoomToSkip = req.zoom + ZOOM_TO_SKIP_FILTER;
    }
    int length ;
    int oldLimit ;
    TIntLongHashMap offsetsMap = new TIntLongHashMap();*/
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
        case OBF::OsmAndPoiIndex::kBoxesFieldNumber:
            {
                auto length = ObfReader::readBigEndianInt(cis);
                auto oldLimit = cis->PushLimit(length);
                //readBoxField(left31, right31, top31, bottom31, 0, 0, 0, offsetsMap,  skipTiles, req, region);
                readPoiTile(reader, section);
                cis->PopLimit(oldLimit);
            }
            break;
        case OBF::OsmAndPoiIndex::kPoiDataFieldNumber:
            /*
            int[] offsets = offsetsMap.keys();
            // also offsets can be randomly skipped by limit
            Arrays.sort(offsets);
            if(skipTiles != null){
                skipTiles.clear();
            }
            LOG.info("Searched poi structure in "+(System.currentTimeMillis() - time) + 
                "ms. Found " + offsets.length +" subtress");
            for (int j = 0; j < offsets.length; j++) {
                codedIS.seek(offsets[j] + indexOffset);
                int len = readInt();
                int oldLim = codedIS.pushLimit(len);
                readPoiData(left31, right31, top31, bottom31, req, region, skipTiles, zoomToSkip);
                codedIS.popLimit(oldLim);
                if(req.isCancelled()){
                    return;
                }
            }
            codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
            */
            return;
        default:
            ObfReader::skipUnknownField(cis, tag);
            break;
        }
    }
}

void OsmAnd::ObfPoiSection::readPoiTile( ObfReader* reader, ObfPoiSection* section )
{
    auto cis = reader->_codedInputStream.get();

    /*req.numberOfReadSubtrees++;
    int zoomToSkip = req.zoom + ZOOM_TO_SKIP_FILTER;
    boolean checkBox = true;
    boolean existsCategories = false;
    int zoom = pzoom;
    int dy = py;
    int dx = px;*/
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
        case OBF::OsmAndPoiBox::kZoomFieldNumber:
            //zoom = codedIS.readUInt32() + pzoom;
            break;
        case OBF::OsmAndPoiBox::kLeftFieldNumber:
            //dx = codedIS.readSInt32();
            break;
        case OBF::OsmAndPoiBox::kTopFieldNumber:
            //dy = codedIS.readSInt32();
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
            /*
            {
                int x = dx + (px << (zoom - pzoom));
                int y = dy + (py << (zoom - pzoom));
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

                int length = readInt();
                int oldLimit = codedIS.pushLimit(length);
                boolean exists = readBoxField(left31, right31, top31, bottom31, x, y, zoom, offsetsMap, skipTiles, req, region);
                codedIS.popLimit(oldLimit);

                if (skipTiles != null && zoom >= zoomToSkip && exists) {
                    long val = ((((long) x) >> (zoom - zoomToSkip)) << zoomToSkip) | (((long) y) >> (zoom - zoomToSkip));
                    if(skipTiles.contains(val)){
                        codedIS.skipRawBytes(codedIS.getBytesUntilLimit());
                        return true;
                    }
                }
            }
            */
            break;
        case OBF::OsmAndPoiBox::kShiftToDataFieldNumber:
            {
                /*
                int x = dx + (px << (zoom - pzoom));
                int y = dy + (py << (zoom - pzoom));
                long l = ((((x << zoom) | y) << 5) | zoom);
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
