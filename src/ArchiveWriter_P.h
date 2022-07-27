#ifndef _OSMAND_CORE_ARCHIVE_READER_P_H_
#define _OSMAND_CORE_ARCHIVE_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QString>
#include <QList>
#include <QIODevice>

#include <libarchive/archive.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "ArchiveWriter.h"

namespace OsmAnd
{
    class ArchiveWriter;
    class ArchiveWriter_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ArchiveWriter_P);
    public:

    private:
    protected:
        ArchiveWriter_P(ArchiveWriter* const owner);
    public:
        virtual ~ArchiveWriter_P();

        ImplementationInterface<ArchiveWriter> owner;
        void createArchive(bool* const ok_, const QString& filePath, const QList<QString>& filesToArcive, const QString& basePath, const bool gzip = false);

    friend class OsmAnd::ArchiveWriter;
    };
}

#endif // !defined(_OSMAND_CORE_ARCHIVE_READER_P_H_)
