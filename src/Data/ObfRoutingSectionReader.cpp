#include "ObfRoutingSectionReader.h"
#include "ObfRoutingSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfRoutingSectionReader::ObfRoutingSectionReader()
{
}

OsmAnd::ObfRoutingSectionReader::~ObfRoutingSectionReader()
{
}

void OsmAnd::ObfRoutingSectionReader::querySubsections(
    const std::shared_ptr<ObfReader>& reader, const QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >& in,
    QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut /*= nullptr*/, IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor /*= nullptr*/ )
{
    ObfRoutingSectionReader_P::querySubsections(*reader->_p, in, resultOut, filter, visitor);
}

void OsmAnd::ObfRoutingSectionReader::loadSubsectionData(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
    QList< std::shared_ptr<const Model::Road> >* resultOut /*= nullptr*/, QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/, std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> visitor /*= nullptr*/ )
{
    ObfRoutingSectionReader_P::loadSubsectionData(*reader->_p, subsection, resultOut, resultMapOut, filter, visitor);
}

void OsmAnd::ObfRoutingSectionReader::loadSubsectionBorderBoxLinesPoints(
    const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
    QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut /*= nullptr*/, IQueryFilter* filter /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine /*= nullptr*/,
    std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint /*= nullptr*/ )
{
    ObfRoutingSectionReader_P::loadSubsectionBorderBoxLinesPoints(*reader->_p, section, resultOut, filter, visitorLine);
}
