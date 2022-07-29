#include "ArchiveWriter.h"
#include "ArchiveWriter_P.h"

OsmAnd::ArchiveWriter::ArchiveWriter()
    : _p(new ArchiveWriter_P(this))
{
}

OsmAnd::ArchiveWriter::~ArchiveWriter()
{
}

void OsmAnd::ArchiveWriter::createArchive(bool* const ok_, const QString& filePath, const QList<QString>& filesToArcive, const QString& basePath, const bool gzip /*= false*/)
{
    _p->createArchive(ok_, filePath, filesToArcive, basePath, gzip);
}
