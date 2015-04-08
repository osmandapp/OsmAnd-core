#include "ObfPoiSectionReader.h"
#include "ObfPoiSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfPoiSectionReader::ObfPoiSectionReader()
{
}

OsmAnd::ObfPoiSectionReader::~ObfPoiSectionReader()
{
}

void OsmAnd::ObfPoiSectionReader::loadCategories(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    std::shared_ptr<const ObfPoiSectionCategories>& outCategories,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfPoiSectionReader_P::loadCategories(*reader->_p, section, outCategories, queryController);
}

void OsmAnd::ObfPoiSectionReader::loadSubtypes(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    std::shared_ptr<const ObfPoiSectionSubtypes>& outSubtypes,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfPoiSectionReader_P::loadSubtypes(*reader->_p, section, outSubtypes, queryController);
}

void OsmAnd::ObfPoiSectionReader::loadAmenities(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const ZoomLevel minZoom /*= MinZoomLevel*/,
    const ZoomLevel maxZoom /*= MaxZoomLevel*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const QSet<ObfPoiCategoryId>* const categoriesFilter /*= nullptr*/,
    const ObfPoiSectionReader::VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfPoiSectionReader_P::loadAmenities(
        *reader->_p,
        section,
        outAmenities,
        minZoom,
        maxZoom,
        bbox31,
        categoriesFilter,
        visitor,
        queryController);
}

void OsmAnd::ObfPoiSectionReader::scanAmenitiesByName(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfPoiSectionInfo>& section,
    const QString& query, QList< std::shared_ptr<const OsmAnd::Amenity> >* outAmenities,
    const ZoomLevel minZoom /*= MinZoomLevel*/,
    const ZoomLevel maxZoom /*= MaxZoomLevel*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const QSet<ObfPoiCategoryId>* const categoriesFilter /*= nullptr*/,
    const ObfPoiSectionReader::VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfPoiSectionReader_P::scanAmenitiesByName(
        *reader->_p,
        section,
        query,
        outAmenities,
        minZoom,
        maxZoom,
        bbox31,
        categoriesFilter,
        visitor,
        queryController);
}
