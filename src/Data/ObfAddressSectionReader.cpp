#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionReader_P.h"

#include "ObfReader.h"
#include "ObfReader_P.h"

OsmAnd::ObfAddressSectionReader::ObfAddressSectionReader()
{
}

OsmAnd::ObfAddressSectionReader::~ObfAddressSectionReader()
{
}

void OsmAnd::ObfAddressSectionReader::loadStreetGroups(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const StreetGroup> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::StreetGroup>&)> visitor /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/, QSet<ObfAddressBlockType>* blockTypeFilter /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadStreetGroups(*reader->_p, section, resultOut, visitor, controller, blockTypeFilter);
}

void OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const StreetGroup>& group,
    QList< std::shared_ptr<const Street> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Street>&)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadStreetsFromGroup(*reader->_p, group, resultOut, visitor, controller);
}

void OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const Building> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Building>&)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadBuildingsFromStreet(*reader->_p, street, resultOut, visitor, controller);
}

void OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Street>& street,
    QList< std::shared_ptr<const StreetIntersection> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::StreetIntersection>&)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadIntersectionsFromStreet(*reader->_p, street, resultOut, visitor, controller);
}
