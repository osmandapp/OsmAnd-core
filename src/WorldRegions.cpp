#include "WorldRegions.h"
#include "WorldRegions_P.h"

const QString OsmAnd::WorldRegions::AntarcticaRegionId(QLatin1String("antarctica"));
const QString OsmAnd::WorldRegions::AfricaRegionId(QLatin1String("africa"));
const QString OsmAnd::WorldRegions::AsiaRegionId(QLatin1String("asia"));
const QString OsmAnd::WorldRegions::AustraliaAndOceaniaRegionId(QLatin1String("australia-oceania-all"));
const QString OsmAnd::WorldRegions::CentralAmericaRegionId(QLatin1String("centralamerica"));
const QString OsmAnd::WorldRegions::EuropeRegionId(QLatin1String("europe"));
const QString OsmAnd::WorldRegions::NorthAmericaRegionId(QLatin1String("northamerica"));
const QString OsmAnd::WorldRegions::RussiaRegionId(QLatin1String("russia"));
const QString OsmAnd::WorldRegions::SouthAmericaRegionId(QLatin1String("southamerica"));
const QString OsmAnd::WorldRegions::NauticalRegionId(QLatin1String("nautical"));
const QString OsmAnd::WorldRegions::OthersRegionId(QLatin1String("others"));

OsmAnd::WorldRegions::WorldRegions(const QString& ocbfFileName_)
    : _p(new WorldRegions_P(this))
    , ocbfFileName(ocbfFileName_)
{
}

OsmAnd::WorldRegions::~WorldRegions()
{
}

bool OsmAnd::WorldRegions::loadWorldRegions(
    QList< std::shared_ptr<const WorldRegion> >* const outRegions,
    const bool keepMapObjects /*= false*/,
    const AreaI* const bbox31 /*= nullptr*/,
    const VisitorFunction visitor /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return _p->loadWorldRegions(
        outRegions,
        keepMapObjects,
        bbox31,
        visitor,
        queryController);
}
