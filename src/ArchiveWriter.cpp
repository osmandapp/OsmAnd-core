#include "ArchiveWriter.h"
#include "ArchiveWriter_P.h"

OsmAnd::ArchiveWriter::ArchiveWriter()
    : _p(new ArchiveWriter_P(this))
{
}

OsmAnd::ArchiveWriter::~ArchiveWriter()
{
}

void OsmAnd::ArchiveWriter::createArchive(bool* const ok_, const QString& filePath, const QList<QString>& filesToArchive, const QString& basePath, const bool gzip /*= false*/)
{
    _p->createArchive(ok_, filePath, filesToArchive, basePath, gzip);
}

QByteArray OsmAnd::ArchiveWriter::createArchive(bool* const ok_, const QList<QString>& filesToArchive, const QString& basePath, const bool gzip /*= false*/)
{
    return _p->createArchive(ok_, filesToArchive, basePath, gzip);
}
