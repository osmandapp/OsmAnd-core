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
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
    QList< std::shared_ptr<const Model::StreetGroup> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/, QSet<ObfAddressBlockType>* blockTypeFilter /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadStreetGroups(reader->_d, section, resultOut, visitor, controller, blockTypeFilter);
}

void OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
    QList< std::shared_ptr<const Model::Street> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadStreetsFromGroup(reader->_d, group, resultOut, visitor, controller);
}

void OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::Street>& street,
    QList< std::shared_ptr<const Model::Building> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadBuildingsFromStreet(reader->_d, street, resultOut, visitor, controller);
}

void OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::Street>& street,
    QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor /*= nullptr*/, const IQueryController* const controller /*= nullptr*/ )
{
    ObfAddressSectionReader_P::loadIntersectionsFromStreet(reader->_d, street, resultOut, visitor, controller);
}
