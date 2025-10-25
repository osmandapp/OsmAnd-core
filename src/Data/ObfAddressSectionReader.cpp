#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfAddressSectionReader::ObfAddressSectionReader()
{
}

OsmAnd::ObfAddressSectionReader::~ObfAddressSectionReader()
{
}

void OsmAnd::ObfAddressSectionReader::loadStreetGroups(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const StreetGroup> >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter /*= fullObfAddressStreetGroupTypesMask()*/,
    const StreetGroupVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadStreetGroups(
        *reader->_p,
        section,
        resultOut,
        bbox31,
        streetGroupTypesFilter,
        visitor,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const StreetGroup>& streetGroup,
    QList< std::shared_ptr<const Street> >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const StreetVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadStreetsFromGroup(
        *reader->_p,
        streetGroup,
        resultOut,
        bbox31,
        visitor,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const Building> >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const BuildingVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadBuildingsFromStreet(
        *reader->_p,
        street,
        resultOut,
        bbox31,
        visitor,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const Street> >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const IntersectionVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadIntersectionsFromStreet(
        *reader->_p,
        street,
        resultOut,
        bbox31,
        visitor,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::scanAddressesByName(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    const QString& query,
    const StringMatcherMode matcherMode,
    QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
    const AreaI* const bbox31 /*= nullptr*/,
    const ObfAddressStreetGroupTypesMask streetGroupTypesFilter /*= fullObfAddressStreetGroupTypesMask()*/,
    const bool includeStreets /*= true*/,
    const bool strictMatch /*= false*/,
    const ObfAddressSectionReader::VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::scanAddressesByName(
        *reader->_p,
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
}
