#include "ObfTransportSectionReader.h"
#include "ObfTransportSectionReader_P.h"

#include "ObfReader.h"

OsmAnd::ObfTransportSectionReader::ObfTransportSectionReader()
{
}

OsmAnd::ObfTransportSectionReader::~ObfTransportSectionReader()
{
}

void OsmAnd::ObfTransportSectionReader::initializeStringTable(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfTransportSectionInfo>& section,
    ObfSectionInfo::StringTable* const stringTable)
{
    ObfTransportSectionReader_P::initializeStringTable(*reader->_p, section, stringTable);
}

void OsmAnd::ObfTransportSectionReader::initializeNames(
    ObfSectionInfo::StringTable* const stringTable,
    std::shared_ptr<TransportStop> s)
{
    ObfTransportSectionReader_P::initializeNames(stringTable, s);
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

