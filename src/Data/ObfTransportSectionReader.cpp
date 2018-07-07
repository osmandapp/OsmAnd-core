#include "ObfTransportSectionReader.h"
#include "ObfTransportSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfTransportSectionReader::ObfTransportSectionReader()
{
}

OsmAnd::ObfTransportSectionReader::~ObfTransportSectionReader()
{
}

void OsmAnd::ObfTransportSectionReader::searchTransportStops(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    QList< std::shared_ptr<const TransportStop> >* resultOut /*= nullptr*/,
    const AreaI* const bbox31 /*= nullptr*/,
    ObfSectionInfo::StringTable* const stringTable /*= nullptr*/,
    const TransportStopVisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfTransportSectionReader_P::searchTransportStops(
                                                *reader->_p,
                                                section,
                                                resultOut,
                                                bbox31,
                                                stringTable,
                                                visitor,
                                                queryController);
}

