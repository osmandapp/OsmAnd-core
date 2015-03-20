#include "ISearch.h"

OsmAnd::ISearch::ISearch()
{
}

OsmAnd::ISearch::~ISearch()
{
}

OsmAnd::ISearch::Criteria::Criteria()
    : minZoomLevel(MinZoomLevel)
    , maxZoomLevel(MaxZoomLevel)
{
}

OsmAnd::ISearch::Criteria::~Criteria()
{
}

OsmAnd::ISearch::IResultEntry::IResultEntry()
{
}

OsmAnd::ISearch::IResultEntry::~IResultEntry()
{
}
