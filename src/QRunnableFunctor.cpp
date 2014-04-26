#include "QRunnableFunctor.h"

OsmAnd::QRunnableFunctor::QRunnableFunctor(const Callback callback_)
    : _callback(callback_)
{
}

OsmAnd::QRunnableFunctor::~QRunnableFunctor()
{
}

void OsmAnd::QRunnableFunctor::run()
{
    _callback(this);
}
