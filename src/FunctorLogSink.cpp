#include "FunctorLogSink.h"

OsmAnd::FunctorLogSink::FunctorLogSink(const WriteCallback writeFunctor_, const FlushCallback flushFunctor_)
    : _writeFunctor(writeFunctor_)
    , _flushFunctor(flushFunctor_)
{
}

OsmAnd::FunctorLogSink::~FunctorLogSink()
{
}

void OsmAnd::FunctorLogSink::log(const LogSeverityLevel level, const char* format, va_list args)
{
    if(!_writeFunctor)
        return;

    _writeFunctor(this, level, format, args);
}

void OsmAnd::FunctorLogSink::flush()
{
    if(!_flushFunctor)
        return;

    _flushFunctor(this);
}
