#include "WorldRegions.h"
#include "WorldRegions_P.h"

const QString OsmAnd::WorldRegions::AfricaRegionId(QLatin1String("africa"));
const QString OsmAnd::WorldRegions::AsiaRegionId(QLatin1String("asia"));
const QString OsmAnd::WorldRegions::AustraliaAndOceaniaRegionId(QLatin1String("australia-oceania"));
const QString OsmAnd::WorldRegions::CentralAmericaRegionId(QLatin1String("centralamerica"));
const QString OsmAnd::WorldRegions::EuropeRegionId(QLatin1String("europe"));
const QString OsmAnd::WorldRegions::NorthAmericaRegionId(QLatin1String("northamerica"));
const QString OsmAnd::WorldRegions::SouthAmericaRegionId(QLatin1String("southamerica"));

OsmAnd::WorldRegions::WorldRegions(const QString& ocbfFileName_)
    : _p(new WorldRegions_P(this))
    , ocbfFileName(ocbfFileName_)
{
}

OsmAnd::WorldRegions::~WorldRegions()
{
}

bool OsmAnd::WorldRegions::loadWorldRegions(
    QVector< std::shared_ptr<const WorldRegion> >& outRegions,
    const IQueryController* const controller /*= nullptr*/) const
{
    return _p->loadWorldRegions(outRegions, controller);
}

OsmAnd::WorldRegions::WorldRegion::WorldRegion(
    const QString& id_,
    const QString& name_,
    const QHash<QString, QString>& localizedNames_,
    const QString& parentId_ /*= QString::null*/)
    : id(id_)
    , name(name_)
    , localizedNames(localizedNames_)
    , parentId(parentId_)
{
}

OsmAnd::WorldRegions::WorldRegion::~WorldRegion()
{
}
