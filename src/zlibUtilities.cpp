#include "zlibUtilities.h"

#include "ignore_warnings_on_external_includes.h"
#define ZLIB_CONST
#include <zlib.h>
#include "restore_internal_warnings.h"


OsmAnd::zlibUtilities::zlibUtilities()
{
}

OsmAnd::zlibUtilities::~zlibUtilities()
{
}

QByteArray OsmAnd::zlibUtilities::decompress(const QByteArray& input, const int windowBits)
{
    QByteArray output(input.size() * 6, Qt::Initialization::Uninitialized);

    z_stream stream;
    memset(&stream, 0, sizeof(z_stream));
    for (;;)
    {
        stream.next_in = reinterpret_cast<const Bytef*>(input.constData());
        stream.avail_in = input.size();

        stream.next_out = reinterpret_cast<Bytef*>(output.data());
        stream.avail_out = output.size();

        int err;
        err = inflateInit2(&stream, windowBits);
        if (err != Z_OK)
            return QByteArray();

        err = inflate(&stream, Z_FINISH);
        if (err != Z_STREAM_END)
        {
            inflateEnd(&stream);

            if (err == Z_BUF_ERROR)
            {
                output.resize(output.size() * 2);
                continue;
            }

            return QByteArray();
        }

        output.resize(stream.total_out);

        inflateEnd(&stream);
        break;
    }

    return output;
}

QByteArray OsmAnd::zlibUtilities::zlibDecompress(const QByteArray& input)
{
    return decompress(input, MAX_WBITS);
}

QByteArray OsmAnd::zlibUtilities::deflateDecompress(const QByteArray& input)
{
    return decompress(input, -MAX_WBITS);
}

QByteArray OsmAnd::zlibUtilities::gzipDecompress(const QByteArray& input)
{
    return decompress(input, MAX_WBITS | 16);
}
