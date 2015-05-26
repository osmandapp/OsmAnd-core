#ifndef _OSMAND_CORE_ZLIB_UTILITIES_H_
#define _OSMAND_CORE_ZLIB_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API zlibUtilities Q_DECL_FINAL
    {
        static QByteArray decompress(const QByteArray& input, const int windowBits);
        static QByteArray zlibDecompress(const QByteArray& input);
        static QByteArray deflateDecompress(const QByteArray& input);
        static QByteArray gzipDecompress(const QByteArray& input);
    private:
        zlibUtilities();
        ~zlibUtilities();
    };
}

#endif // !defined(_OSMAND_CORE_ZLIB_UTILITIES_H_)
