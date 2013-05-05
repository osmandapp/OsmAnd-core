#include "PoiDirectoryContext.h"

OsmAnd::PoiDirectoryContext::PoiDirectoryContext( const QList< std::shared_ptr<ObfReader> >& sources )
    : sources(sources)
{
}

OsmAnd::PoiDirectoryContext::~PoiDirectoryContext()
{
}
