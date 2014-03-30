#include "LambdaLogSink.h"

OsmAnd::LambdaLogSink::LambdaLogSink(const WriteHandlerSignature writeHandler, const FlushHandlerSignature flushHandler)
    : _writeHandler(writeHandler)
    , _flushHandler(flushHandler)
{
}

OsmAnd::LambdaLogSink::~LambdaLogSink()
{
}

void OsmAnd::LambdaLogSink::log(const LogSeverityLevel level, const char* format, va_list args)
{
    if(!_writeHandler)
        return;

    _writeHandler(this, level, format, args);
}

void OsmAnd::LambdaLogSink::flush()
{
    if(!_flushHandler)
        return;

    _flushHandler(this);
}
