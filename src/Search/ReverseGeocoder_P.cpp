#include "ReverseGeocoder_P.h"

#include "AddressesByNameSearch.h"
#include "Building.h"
#include "Logging.h"
#include "ObfDataInterface.h"
#include "Road.h"
#include "Utilities.h"

#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Search/CommonWords.h>

#include <QStringBuilder>

//
//  OsmAnd-java/src/net/osmand/binary/GeocodingUtilities.java
//  git revision c0fc23f823862a2290e84577b6a944e49cc87c33
//

// Location to test parameters http://www.openstreetmap.org/#map=18/53.896473/27.540071 (hno 44)
const float THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER = 5;
const float STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS = 250;
const float STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS = 400;

const float DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME = 1000;
const float DISTANCE_STREET_NAME_PROXIMITY_BY_NAME = 15000;

const float THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER = 1.5f;
const float DISTANCE_BUILDING_PROXIMITY = 100;

OsmAnd::ReverseGeocoder_P::ReverseGeocoder_P(
        OsmAnd::ReverseGeocoder* owner_,
        const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator_)
    : roadLocator(roadLocator_)
    , addressByNameSearch(std::make_shared<AddressesByNameSearch>(owner_->obfsCollection))
    , owner(owner_)
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
    return a->getDistance() < b->getDistance();
}

void addWord(QStringList &ls, QString word, bool addCommonWords)
{
    const QString w = word.trimmed().toLower();
    if (!w.isEmpty()) {
        if (!addCommonWords && OsmAnd::CommonWords::getCommonGeocoding(w) != -1)
            return;
        
        ls << w;
    }
}

QStringList prepareStreetName(const QString &s, bool addCommonWords)
{
    QStringList ls;
    int beginning = 0;
    for (int i = 1; i < s.length(); i++)
    {
        if (s[i] == ' ')
        {
            addWord(ls, s.mid(beginning, i - beginning), addCommonWords);
            beginning = i;
        }
        else if (s[i] == '(')
        {
            addWord(ls, s.mid(beginning, i - beginning), addCommonWords);
            while (i < s.length())
            {
                auto c = s[i];
                i++;
                beginning = i;
                if (c == ')')
                    break;
            }
            
        }
    }
    if (beginning < s.length()) {
        QString lastWord = s.mid(beginning, s.length() - beginning);
        addWord(ls, lastWord, addCommonWords);
    }
    std::sort(ls.begin(), ls.end(), [](const QString& a, const QString& b) {
        return (a.length() != b.length()) ? (a.length() > b.length()) : (a > b);
    });
    return ls;
}

QString extractMainWord(const QStringList &streetNamesUsed)
{
    QString mainWord = "";
    for (QString word : streetNamesUsed)
        if (word.length() > mainWord.length())
            mainWord = word;

    return mainWord;
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::justifyReverseGeocodingSearch(
        const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>& road,
        double knownMinBuildingDistance) const
{
    QVector<std::shared_ptr<ResultEntry>> streetList{};
    QVector<std::shared_ptr<const ResultEntry>> result{};

    bool addCommonWords = false;
    auto streetNamesUsed = prepareStreetName(road->streetName, addCommonWords);
    if (streetNamesUsed.size() == 0)
    {
        addCommonWords = true;
        streetNamesUsed = prepareStreetName(road->streetName, addCommonWords);
    }
    if (!streetNamesUsed.isEmpty())
    {
        QString mainWord = extractMainWord(streetNamesUsed);
        OsmAnd::AddressesByNameSearch::Criteria criteria;
        criteria.name = mainWord;
        criteria.includeStreets = true;
        criteria.strictMatch = true;
        criteria.streetGroupTypesMask = ObfAddressStreetGroupTypesMask().set(ObfAddressStreetGroupType::CityOrTown);
        criteria.bbox31 = Nullable<AreaI>((AreaI)Utilities::boundingBox31FromAreaInMeters(DISTANCE_STREET_NAME_PROXIMITY_BY_NAME, *road->searchPoint31()));
        addressByNameSearch->performSearch(
                    criteria,
                    [&streetList, addCommonWords, streetNamesUsed, road](const OsmAnd::ISearch::Criteria& criteria,
                    const OsmAnd::BaseSearch::IResultEntry& resultEntry) {
            auto const& address = static_cast<const OsmAnd::AddressesByNameSearch::ResultEntry&>(resultEntry).address;
            if (address->addressType == OsmAnd::AddressType::Street)
            {
                auto const& street = std::static_pointer_cast<const OsmAnd::Street>(address);
                if (prepareStreetName(street->nativeName, addCommonWords) == streetNamesUsed)
                {
                    if (road->searchPoint31().isSet())
                    {
                        double d = Utilities::distance(Utilities::convert31ToLatLon(street->position31), *road->searchPoint);
                        if (d < DISTANCE_STREET_NAME_PROXIMITY_BY_NAME) {
                            const std::shared_ptr<ResultEntry> rs = std::make_shared<ResultEntry>();
                            rs->road = road->road;
                            rs->street = street;
                            rs->streetGroup = street->streetGroup;
                            rs->searchPoint = road->searchPoint;
                            rs->connectionPoint = Utilities::convert31ToLatLon(street->position31);
                            rs->setDistance(d);
                            streetList.append(rs);
                        }
                    }
                }
            }
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
        for (const std::shared_ptr<ResultEntry>& street : streetList)
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
                    knownMinBuildingDistance = streetBuildings[0]->getDistance();

                std::find_if(streetBuildings.begin(), streetBuildings.end(),
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

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::loadStreetBuildings(
        const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> road,
        const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> street) const
{
    QVector<std::shared_ptr<const ResultEntry>> result{};
    const AreaI bbox = (AreaI)Utilities::boundingBox31FromAreaInMeters(DISTANCE_STREET_NAME_PROXIMITY_BY_NAME, *road->searchPoint31());
    auto const& dataInterface = owner->obfsCollection->obtainDataInterface(&bbox);
    QList<std::shared_ptr<const Street>> streets{street->street};
    QHash<std::shared_ptr<const Street>, QList<std::shared_ptr<const Building>>> buildingsForStreet{};
    dataInterface->loadBuildingsFromStreets(streets, &buildingsForStreet);
    auto const& buildings = buildingsForStreet[street->street];
    for (const std::shared_ptr<const Building>& b : buildings)
    {
        auto makeResult = [b, street, &result](){
            auto bld = std::make_shared<ResultEntry>();
            bld->road = street->road;
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
            double coeff = Utilities::projectionCoeff31(*road->searchPoint31(), b->position31, b->interpolationPosition31);
            double plat = s.latitude + (to.latitude - s.latitude) * coeff;
            double plon = s.longitude + (to.longitude - s.longitude) * coeff;
            if (Utilities::distance(road->searchPoint->longitude, road->searchPoint->latitude, plon, plat) < DISTANCE_BUILDING_PROXIMITY)
            {
                auto bld = makeResult();
                auto nm = b->getInterpolationName(coeff);
                if (!nm.isEmpty())
                    bld->buildingInterpolation = nm;
            }
        }
        else if (Utilities::distance(Utilities::convert31ToLatLon(b->position31), *road->searchPoint) < DISTANCE_BUILDING_PROXIMITY)
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
    auto roads = roadLocator->findNearestRoads(searchPoint31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * 2, OsmAnd::RoutingDataLevel::Detailed,
                                               []
                                               (const std::shared_ptr<const OsmAnd::Road>& road) -> bool
                                               {
                                                   return !road->captions.isEmpty();
                                               });
    if (roads.isEmpty())
        roads = roadLocator->findNearestRoads(searchPoint31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * 10, OsmAnd::RoutingDataLevel::Detailed,
                                              []
                                              (const std::shared_ptr<const OsmAnd::Road>& road) -> bool
                                              {
                                                  return !road->captions.isEmpty();
                                              });
    
    double distSquare = 0;
    QSet<ObfObjectId> set{};
    QSet<QString> streetNames{};
    for (auto p : roads)
    {
        auto road = p.first;
        double roadDistSquare = p.second->distSquare;
        if (set.contains(road->id))
            continue;
        else
            set.insert(road->id);
        if (!road->captions.isEmpty())
        {
            if (distSquare == 0 || distSquare > roadDistSquare)
                distSquare = roadDistSquare;
            std::shared_ptr<ResultEntry> entry = std::make_shared<ResultEntry>();
            entry->road = road;
            entry->streetName = road->getCaptionInNativeLanguage();
            if (entry->streetName.isEmpty())
            {
                if (!road->captions.isEmpty())
                    entry->streetName = road->captions.values().last();
            }
                
            entry->searchPoint = searchPoint;
            entry->connectionPoint = LatLon(Utilities::get31LatitudeY(p.second->preciseY), Utilities::get31LongitudeX(p.second->preciseX));
            
            if (!streetNames.contains(entry->streetName))
            {
                streetNames.insert(entry->streetName);
                result.append(entry);
            }
        }
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS, 2) &&
                distSquare != 0 && roadDistSquare > THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER * distSquare)
            break;
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2))
            break;
    }
    std::sort(result.begin(), result.end(), DISTANCE_COMPARATOR);
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
            minBuildingDistance = (minBuildingDistance == 0) ? md : std::min(md, minBuildingDistance);
            complete.append(justified);
        }
    }
    std::sort(complete.begin(), complete.end(), DISTANCE_COMPARATOR);
    return !complete.isEmpty() ? complete[0] : std::make_shared<ResultEntry>();
}
