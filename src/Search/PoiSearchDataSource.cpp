#include "PoiSearchDataSource.h"
#include "PoiSearchDataSource_P.h"

OsmAnd::PoiSearchDataSource::PoiSearchDataSource(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : _p(new PoiSearchDataSource_P(this))
    , obfsCollection(obfsCollection_)
{
}

OsmAnd::PoiSearchDataSource::~PoiSearchDataSource()
{
}
