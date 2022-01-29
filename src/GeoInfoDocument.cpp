#include "GeoInfoDocument.h"

#include "QKeyValueIterator.h"

OsmAnd::Extensions::Extension::Extension()
{
}

OsmAnd::Extensions::Extension::~Extension()
{
}

QHash<QString, QVariant> OsmAnd::Extensions::Extension::getValues(const bool recursive /*= true*/) const
{
    QHash<QString, QVariant> values;

    if (!value.isEmpty())
        values.insert(name, value);

    const auto prefix = name + QStringLiteral(":");

    for (const auto attributeEntry : rangeOf(constOf(attributes)))
        values.insert(prefix + attributeEntry.key(), attributeEntry.value());

    if (recursive)
    {
        for (const auto& subextension : constOf(subextensions))
        {
            const auto subvalues = subextension->getValues();
            for (const auto attributeEntry : rangeOf(constOf(subvalues)))
                values.insert(prefix + attributeEntry.key(), attributeEntry.value());
        }
    }

    return values;
}

OsmAnd::Extensions::Extensions()
{
}

OsmAnd::Extensions::~Extensions()
{
}

QHash<QString, QVariant> OsmAnd::Extensions::getValues(const bool recursive /*= true*/) const
{
    QHash<QString, QVariant> values;

    if (!value.isEmpty())
        values.insert(QString::null, value);

    for (const auto attributeEntry : rangeOf(constOf(attributes)))
        values.insert(attributeEntry.key(), attributeEntry.value());

    if (recursive)
    {
        for (const auto& subextension : constOf(extensions))
            values.unite(subextension->getValues());
    }

    return values;
}

OsmAnd::GeoInfoDocument::GeoInfoDocument()
{
}

OsmAnd::GeoInfoDocument::~GeoInfoDocument()
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

OsmAnd::GeoInfoDocument::Author::Author()
{
}

OsmAnd::GeoInfoDocument::Author::~Author()
{
}

OsmAnd::GeoInfoDocument::Copyright::Copyright()
{
}

OsmAnd::GeoInfoDocument::Copyright::~Copyright()
{
}

OsmAnd::GeoInfoDocument::Bounds::Bounds()
    : minlat(std::numeric_limits<double>::quiet_NaN())
    , minlon(std::numeric_limits<double>::quiet_NaN())
    , maxlat(std::numeric_limits<double>::quiet_NaN())
    , maxlon(std::numeric_limits<double>::quiet_NaN())
{
}

OsmAnd::GeoInfoDocument::Bounds::~Bounds()
{
}

OsmAnd::GeoInfoDocument::WptPt::WptPt()
    : position(LatLon(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()))
    , elevation(std::numeric_limits<double>::quiet_NaN())
    , horizontalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , verticalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
{
}

OsmAnd::GeoInfoDocument::WptPt::~WptPt()
{
}

OsmAnd::GeoInfoDocument::RouteSegment::RouteSegment()
{
}

OsmAnd::GeoInfoDocument::RouteSegment::~RouteSegment()
{
}

OsmAnd::GeoInfoDocument::RouteType::RouteType()
{
}

OsmAnd::GeoInfoDocument::RouteType::~RouteType()
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

OsmAnd::GeoInfoDocument::Route::Route()
{
}

OsmAnd::GeoInfoDocument::Route::~Route()
{
}

bool OsmAnd::GeoInfoDocument::hasRtePt() const
{
    for (auto& r : routes)
        if (r->points.size() > 0)
            return true;

    return false;
}

bool OsmAnd::GeoInfoDocument::hasWptPt() const
{
    return points.size() > 0;
}

bool OsmAnd::GeoInfoDocument::hasTrkPt() const
{
    for (auto& t : tracks)
        for (auto& ts : t->segments)
            if (ts->points.size() > 0)
                return true;

    return false;
}
