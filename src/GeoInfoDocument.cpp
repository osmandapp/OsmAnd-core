#include "GeoInfoDocument.h"

OsmAnd::GeoInfoDocument::GeoInfoDocument()
{
}

OsmAnd::GeoInfoDocument::~GeoInfoDocument()
{
}

OsmAnd::GeoInfoDocument::ExtraData::ExtraData()
{
}

OsmAnd::GeoInfoDocument::ExtraData::~ExtraData()
{
}

OsmAnd::GeoInfoDocument::Link::Link()
{
}

OsmAnd::GeoInfoDocument::Link::~Link()
{
}

OsmAnd::GeoInfoDocument::Metadata::Metadata()
{
}

OsmAnd::GeoInfoDocument::Metadata::~Metadata()
{
}

OsmAnd::GeoInfoDocument::LocationMark::LocationMark()
    : position(LatLon(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()))
    , elevation(std::numeric_limits<double>::quiet_NaN())
{
}

OsmAnd::GeoInfoDocument::LocationMark::~LocationMark()
{
}

OsmAnd::GeoInfoDocument::TrackPoint::TrackPoint()
{
}

OsmAnd::GeoInfoDocument::TrackPoint::~TrackPoint()
{
}

OsmAnd::GeoInfoDocument::TrackSegment::TrackSegment()
{
}

OsmAnd::GeoInfoDocument::TrackSegment::~TrackSegment()
{
}

OsmAnd::GeoInfoDocument::Track::Track()
{
}

OsmAnd::GeoInfoDocument::Track::~Track()
{
}

OsmAnd::GeoInfoDocument::RoutePoint::RoutePoint()
{
}

OsmAnd::GeoInfoDocument::RoutePoint::~RoutePoint()
{
}

OsmAnd::GeoInfoDocument::Route::Route()
{
}

OsmAnd::GeoInfoDocument::Route::~Route()
{
}
