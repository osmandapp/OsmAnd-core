#include "ReverseGeocoder_P.h"

#include "AddressesByNameSearch.h"
#include "Building.h"
#include "Logging.h"
#include "ObfDataInterface.h"
#include "Road.h"
#include "Utilities.h"

#include <QStringBuilder>

// Location to test parameters http://www.openstreetmap.org/#map=18/53.896473/27.540071 (hno 44)
const float THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER = 5;
const float STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS = 250;
const float STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS = 400;

const float DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME = 1000;

const float THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER = 1.5f;
const float DISTANCE_BUILDING_PROXIMITY = 100;

const QStringList SUFFIXES {
        QStringLiteral("av."),
        QStringLiteral("avenue"),
        QStringLiteral("просп."),
        QStringLiteral("пер."),
        QStringLiteral("пр."),
        QStringLiteral("заул."),
        QStringLiteral("проспект"),
        QStringLiteral("переул."),
        QStringLiteral("бул."),
        QStringLiteral("бульвар"),
        QStringLiteral("тракт")
};
const QStringList DEFAULT_SUFFIXES {
        QStringLiteral("str."),
        QStringLiteral("street"),
        QStringLiteral("улица"),
        QStringLiteral("ул."),
        QStringLiteral("вулица"),
        QStringLiteral("вул."),
        QStringLiteral("вулиця")
};

OsmAnd::ReverseGeocoder_P::ReverseGeocoder_P(
        OsmAnd::ReverseGeocoder* owner_,
        const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator_)
    : owner(owner_)
    , roadLocator(roadLocator_)
    , addressByNameSearch(std::make_shared<AddressesByNameSearch>(owner_->obfsCollection))
{

}

OsmAnd::ReverseGeocoder_P::~ReverseGeocoder_P()
{

}

void OsmAnd::ReverseGeocoder_P::performSearch(
    const ISearch::Criteria& criteria_,
    const ISearch::NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);
    if (!criteria.latLon.isSet() && !criteria.position31.isSet())
        return;
    auto searchPoint = criteria.latLon.isSet() ? *criteria.latLon : Utilities::convert31ToLatLon(*criteria.position31);
    QVector<std::shared_ptr<const ResultEntry>> roads = reverseGeocodeToRoads(searchPoint);
    std::shared_ptr<const ResultEntry> result = justifyResult(roads);
    newResultEntryCallback(criteria, *result);
}

bool OsmAnd::ReverseGeocoder_P::DISTANCE_COMPARATOR(
        const std::shared_ptr<const ResultEntry>& a,
        const std::shared_ptr<const ResultEntry>& b)
{
    return a->getDistance() > b->getDistance();
}

QStringList splitToWordsOrderedByLength(const QString &streetName)
{
    QStringList result = streetName.split("[ ()]");
    result.erase(std::remove_if(result.begin(), result.end(), [](const QString& word){
        return DEFAULT_SUFFIXES.contains(word);
    }), result.end());
    std::sort(result.begin(), result.end(), [](const QString& a, const QString& b){
        return (a.length() != b.length()) ? (a.length() > b.length()) : (a > b);
    });
    return result;
}

QString extractMainWord(const QStringList &streetNamePacked)
{
    for (QString word : streetNamePacked)
        if (!SUFFIXES.contains(word))
            return word;
    return streetNamePacked[0];
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::justifyReverseGeocodingSearch(
        const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>& road,
        double knownMinBuildingDistance) const
{
    QVector<std::shared_ptr<ResultEntry>> streetList{};
    QVector<std::shared_ptr<const ResultEntry>> result{};
    QStringList streetNamePacked = splitToWordsOrderedByLength(road->streetName);
    if (!streetNamePacked.isEmpty())
    {
        QString log = QStringLiteral("Search street by name ") % road->streetName % QStringLiteral(" ") % streetNamePacked.join(",");
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, log.toLatin1());
        QString mainWord = extractMainWord(streetNamePacked);
        OsmAnd::AddressesByNameSearch::Criteria criteria;
        criteria.name = mainWord;
        criteria.includeStreets = true;
        criteria.streetGroupTypesMask = OsmAnd::ObfAddressStreetGroupTypesMask{};
        criteria.bbox31 = road->searchBbox();
        addressByNameSearch->performSearch(
                    criteria,
                    [&streetList, streetNamePacked, road](const OsmAnd::ISearch::Criteria& criteria,
                    const OsmAnd::BaseSearch::IResultEntry& resultEntry) {
            auto const& address = static_cast<const OsmAnd::AddressesByNameSearch::ResultEntry&>(resultEntry).address;
            auto const& street = std::static_pointer_cast<const OsmAnd::Street>(address);
            if (splitToWordsOrderedByLength(street->nativeName) == streetNamePacked)
            {
////                double d = Utilities::distance31(street->position31, position31);
////                if (d < DISTANCE_STREET_NAME_PROXIMITY_BY_NAME) {
                const std::shared_ptr<ResultEntry> rs = std::make_shared<ResultEntry>();
                rs->searchPoint = road->searchPoint;
                rs->street = street;
                // set connection point to sort
                rs->connectionPoint = Utilities::convert31ToLatLon(street->position31);
                rs->streetGroup = street->streetGroup;
                streetList.append(rs);
//                return true;
////                }
            }
//            return false;
        });
    }

    if (streetList.isEmpty())
    {
        result.append(road);
    }
    else
    {
        std::sort(streetList.begin(), streetList.end(), DISTANCE_COMPARATOR);
        double streetDistance = 0;
        for (const std::shared_ptr<ResultEntry> street : streetList)
        {
            if (streetDistance == 0)
                streetDistance = street->getDistance();
            else if (street->getDistance() > streetDistance + DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME)
                continue;
            street->connectionPoint = road->connectionPoint;
            QVector<std::shared_ptr<const ResultEntry>> streetBuildings = loadStreetBuildings(road, street);
            std::sort(streetBuildings.begin(), streetBuildings.end(), DISTANCE_COMPARATOR);
            if (!streetBuildings.isEmpty())
            {
                if (knownMinBuildingDistance == 0)
                {
                    knownMinBuildingDistance = streetBuildings[0]->getDistance();
                    result.append(streetBuildings[0]);
                }
                std::find_if(std::next(streetBuildings.begin()), streetBuildings.end(),
                             [&result, &knownMinBuildingDistance](const std::shared_ptr<const ResultEntry>& building){
                    bool stop = building->getDistance() > knownMinBuildingDistance * THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER;
                    if (!stop)
                        result.append(building);
                    return stop;
                });
            }
            result.append(street);
        }
    }
    std::sort(result.begin(), result.end(), DISTANCE_COMPARATOR);
    return result;
}

int extractFirstInteger(QString s)
{
    int i = 0;
    for (QChar k : s)
        if (k.isDigit())
            i = i * 10 + k.digitValue();
        else
            break;
    return i;
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::loadStreetBuildings(
        const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> road,
        const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> street) const
{
    QVector<std::shared_ptr<const ResultEntry>> result{};
    const AreaI bbox{road->searchBbox()};
    auto const& dataInterface = owner->obfsCollection->obtainDataInterface(&bbox);
    QList<std::shared_ptr<const Street>> streets{street->street};
    QHash<std::shared_ptr<const Street>, QList<std::shared_ptr<const Building>>> buildingsForStreet{};
    dataInterface->loadBuildingsFromStreets(streets, &buildingsForStreet);
    auto const& buildings = buildingsForStreet[street->street];
    for (const std::shared_ptr<const Building> b : buildings)
    {
        auto makeResult = [b, street, &result](){
            auto bld = std::make_shared<ResultEntry>();
            bld->searchPoint = street->searchPoint;
            bld->street = street->street;
            bld->streetGroup = street->streetGroup;
            bld->building = b;
            bld->connectionPoint = Utilities::convert31ToLatLon(b->position31);
            result.append(std::static_pointer_cast<const ResultEntry>(bld));
            return bld;
        };

        if (b->interpolation != Building::Interpolation::Disabled)
        {
            LatLon s = Utilities::convert31ToLatLon(b->position31);
            LatLon to = Utilities::convert31ToLatLon(b->interpolationPosition31);
            double coeff = Utilities::projection31(*road->searchPoint31(), b->position31, b->interpolationPosition31);
            double plat = s.latitude + (to.latitude - s.latitude) * coeff;
            double plon = s.longitude + (to.longitude - s.longitude) * coeff;
            if (Utilities::distance(road->searchPoint->latitude, road->searchPoint->longitude, plat, plon) < DISTANCE_BUILDING_PROXIMITY)
            {
                auto bld = makeResult();
                if (!b->interpolationNativeName.isEmpty())
                {
                    int fi = extractFirstInteger(b->nativeName);
                    int si = extractFirstInteger(b->interpolationNativeName);
                    if (si != 0 && fi != 0) {
                        int num = (int) (fi + (si - fi) * coeff);
                        if (b->interpolation == Building::Interpolation::Even || b->interpolation == Building::Interpolation::Odd)
                        {
                            if (num % 2 == (b->interpolation == Building::Interpolation::Even ? 1 : 0))
                                num--;
                        }
//                        else if (b->interpolationInterval > 0)
//                        {
//                            int intv = b->interpolationInterval;
//                            if ((num - fi) % intv != 0) {
//                                num = ((num - fi) / intv) * intv + fi;
//                            }
//                        }
                        bld->buildingInterpolation = QString::number(num);
                    }
                }
            }
        }
        else if (Utilities::distance31(b->position31, *road->searchPoint31()) < DISTANCE_BUILDING_PROXIMITY)
        {
            makeResult();
        }
    }
    return result;
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::reverseGeocodeToRoads(
        const LatLon searchPoint) const
{
    QVector<std::shared_ptr<const ResultEntry>> result{};
    auto searchPoint31 = Utilities::convertLatLonTo31(searchPoint);
    auto roads = roadLocator->findNearestRoads(searchPoint31, std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2));
    double distSquare = 0;
    QSet<ObfObjectId> set{};
    for (auto p : roads)
    {
        auto road = p.first;
        double roadDistSquare = p.second;
        if (set.contains(road->id))
            continue;
        else
            set.insert(road->id);
        if (!road->captions.isEmpty())
        {
            if (distSquare == 0 || distSquare > roadDistSquare)
                distSquare = roadDistSquare;
            std::shared_ptr<ResultEntry> entry = std::make_shared<ResultEntry>();
            if (!road->captions.isEmpty())
                entry->streetName = road->captions.values()[0];
            entry->searchPoint = searchPoint;
            entry->dist = std::sqrt(roadDistSquare);
            result.append(entry);
        }
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS, 2) &&
                distSquare != 0 && roadDistSquare > THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER * distSquare)
            break;
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2))
            break;
    }
    return result;
}

std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> OsmAnd::ReverseGeocoder_P::justifyResult(
        QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> res) const
{
    QVector<std::shared_ptr<const ResultEntry>> complete{};
    double minBuildingDistance = 0;
    for (std::shared_ptr<const ResultEntry> r : res)
    {
        QVector<std::shared_ptr<const ResultEntry>> justified = justifyReverseGeocodingSearch(r, minBuildingDistance);
        if (!justified.isEmpty())
        {
            double md = justified[0]->getDistance();
            if (minBuildingDistance == 0)
            {
                minBuildingDistance = md;
            }
            else
            {
                minBuildingDistance = std::min(md, minBuildingDistance);
            }
            complete << justified;
        }
    }
    std::sort(complete.begin(), complete.end(), DISTANCE_COMPARATOR);
    return !complete.isEmpty() ? complete[0] : std::make_shared<ResultEntry>();
}
