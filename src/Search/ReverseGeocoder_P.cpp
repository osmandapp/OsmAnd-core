#include "ReverseGeocoder_P.h"

#include "Address.h"
#include "Building.h"
#include "IQueryController.h"
#include "Logging.h"
#include "ObfDataInterface.h"
#include "ObfInfo.h"
#include "ObfReader.h"
#include "ObfAddressSectionInfo.h"
#include "ObfRoutingSectionInfo.h"
#include "Road.h"
#include "SearchAlgorithms.h"
#include "Utilities.h"

#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Search/CommonWords.h>

#include <QStringBuilder>

//
//  OsmAnd-java/src/main/java/net/osmand/binary/GeocodingUtilities.java
//  git revision 67ad6bf52d (r5.4, #26383)
//

// Location to test parameters http://www.openstreetmap.org/#map=18/53.896473/27.540071 (hno 44)
const float THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER = 5;
const float STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS = 250;
const float STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS = 400;

const float DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME = 7500;
const float DISTANCE_STREET_NAME_PROXIMITY_BY_NAME = 15000;

const float THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER = 1.5f;
const float DISTANCE_BUILDING_PROXIMITY = 100;

// the roads of one lookup lie within a few hundred metres of the point and many share the query word
// ("calle"): the name index is searched once per word around the point, every street name is split once
// and every street has its buildings loaded once
struct OsmAnd::ReverseGeocoder_P::StreetsCache
{
    static constexpr double MARGIN = 1000;

    AreaI bbox31;
    // the maps with addresses around the point, and each of them alone
    std::shared_ptr<ObfDataInterface> dataInterface;
    QHash<const ObfInfo*, std::shared_ptr<ObfDataInterface>> mapDataInterfaces;
    QHash<QPair<QString, const ObfInfo*>, QList<std::shared_ptr<const Street>>> streets;
    QHash<QString, QStringList> words;
    QHash<const Street*, QList<std::shared_ptr<const Building>>> buildings;
};

OsmAnd::ReverseGeocoder_P::ReverseGeocoder_P(
        OsmAnd::ReverseGeocoder* owner_,
        const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator_)
    : owner(owner_)
    , roadLocator(roadLocator_)
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
    if (queryController && queryController->isAborted())
        return;
    auto searchPoint = criteria.latLon.isSet() ? *criteria.latLon : Utilities::convert31ToLatLon(*criteria.position31);
    QVector<std::shared_ptr<const ResultEntry>> roads = reverseGeocodingSearch(searchPoint);
    if (queryController && queryController->isAborted())
        return;
    const auto addresses = findAddresses(roads, queryController);
    if (queryController && queryController->isAborted())
        return;
    newResultEntryCallback(criteria, !addresses.isEmpty() ? *addresses.first() : ResultEntry());
}

static bool DISTANCE_COMPARATOR(const OsmAnd::ReverseGeocoder::ResultEntry& a, const OsmAnd::ReverseGeocoder::ResultEntry& b)
{
    if ((int) a.getDistance() == (int) b.getDistance())
    {
        return a.getCityDistance() < b.getCityDistance();
    }
    return a.getDistance() < b.getDistance();
}

// equal distances keep the order they were added in (buildings before their street), as Collections.sort does
template<typename T>
static void sortByDistance(QVector<std::shared_ptr<T>>& list)
{
    std::stable_sort(list.begin(), list.end(),
        [](const std::shared_ptr<T>& a, const std::shared_ptr<T>& b)
        {
            return DISTANCE_COMPARATOR(*a, *b);
        });
}

static const OsmAnd::ObfInfo* obfOf(const std::shared_ptr<const OsmAnd::ObfSectionInfo>& section)
{
    return section ? section->container.lock().get() : nullptr;
}

static const OsmAnd::ObfInfo* obfOf(const std::shared_ptr<const OsmAnd::Road>& road)
{
    return road ? obfOf(std::static_pointer_cast<const OsmAnd::ObfSectionInfo>(road->section)) : nullptr;
}

// "Tempelhofer Damm" == "Tempelhofer Damm (Tempelhof-Schöneberg)"
static QString stripBraces(const QString& name)
{
    const int i = name.indexOf(QLatin1Char('('));
    if (i < 0)
        return name;
    QString result = name.left(i);
    const int j = name.indexOf(QLatin1Char(')'), i);
    if (j > -1)
        result = (result.trimmed() + QLatin1Char(' ') + name.mid(j + 1)).trimmed();
    return result;
}

static QStringList prepareStreetName(const QString& streetName, bool includeCommonWords)
{
    QStringList words;
    for (const auto& word : OsmAnd::SearchAlgorithms::splitAndNormalize(stripBraces(streetName)))
    {
        if (!word.isEmpty() && (includeCommonWords || OsmAnd::CommonWords::getInstance().getCommonGeocoding(word) == -1))
            words << word;
    }
    return words; // keep original order ("NC 42" - search by "NC" not by "42")
}

static const QStringList& sortedWords(const QString& name, bool includeCommonWords, QHash<QString, QStringList>& cache)
{
    const QString key = (includeCommonWords ? QStringLiteral("+") : QStringLiteral("-")) + name;
    auto it = cache.find(key);
    if (it == cache.end())
    {
        // Strip dashes before split to match "NC 42" == "NC-42"
        auto words = prepareStreetName(QString(name).replace(QLatin1Char('-'), QLatin1Char(' ')), includeCommonWords);
        // the sort only brings both word lists to one order before ==, plain string order is enough
        std::sort(words.begin(), words.end());
        it = cache.insert(key, words);
    }
    return *it;
}

static bool matchStreetName(const QString& s1, const QString& s2, bool matchWithCommonWords, QHash<QString, QStringList>& cache)
{
    if (s1.isEmpty() || s2.isEmpty())
        return false;
    if (s1 == s2)
        return true;

    const auto& s1words = sortedWords(s1, false, cache);
    if (!s1words.isEmpty() && s1words == sortedWords(s2, false, cache))
        return true;

    if (matchWithCommonWords)
    {
        const auto& s1all = sortedWords(s1, true, cache);
        return !s1all.isEmpty() && s1all == sortedWords(s2, true, cache);
    }

    return false;
}

// the street with the name of the road in the address index and the buildings on it within
// DISTANCE_BUILDING_PROXIMITY of the point: buildings first, the street after them, the road
// itself when no street matches
QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::findStreetAndBuildings(
        const std::shared_ptr<const ResultEntry>& road,
        double knownMinBuildingDistance,
        StreetsCache& cache,
        const std::shared_ptr<const IQueryController>& queryController) const
{
    QVector<std::shared_ptr<ResultEntry>> streetsList;
    QVector<std::shared_ptr<const ResultEntry>> res;

    auto streetNamesUsed = prepareStreetName(road->streetName, false);

    bool addCommonWords = false;
    for (const auto& word : constOf(streetNamesUsed))
    {
        if (SearchAlgorithms::isNumber2Letters(word))
        {
            addCommonWords = true; // 1-я Цэнтральная вуліца
            break;
        }
    }
    if (streetNamesUsed.isEmpty() || addCommonWords)
    {
        streetNamesUsed = prepareStreetName(road->streetName, true);
        addCommonWords = true;
    }

    if (!streetNamesUsed.isEmpty())
    {
        QString longestWord;
        for (const auto& word : constOf(streetNamesUsed))
        {
            if (word.length() > longestWord.length())
                longestWord = word;
        }
        // only the road's own map is searched, as the java code searches only the reader of the road's region;
        // a road of a map without addresses (road-only) takes the streets of any map
        const auto roadObf = obfOf(road->road);
        auto itMap = cache.mapDataInterfaces.find(roadObf);
        if (itMap == cache.mapDataInterfaces.end())
        {
            std::shared_ptr<ObfDataInterface> mapDataInterface;
            if (cache.dataInterface)
            {
                for (const auto& obfReader : constOf(cache.dataInterface->obfReaders))
                {
                    if (roadObf && obfReader->obtainInfo().get() == roadObf)
                    {
                        mapDataInterface = std::make_shared<ObfDataInterface>(QList<std::shared_ptr<const ObfReader>>{ obfReader });
                        break;
                    }
                }
            }
            itMap = cache.mapDataInterfaces.insert(roadObf, mapDataInterface);
        }
        const auto key = qMakePair(longestWord, *itMap ? roadObf : nullptr);
        auto itStreets = cache.streets.find(key);
        if (itStreets == cache.streets.end())
        {
            QList<std::shared_ptr<const Street>> found;
            const auto& dataInterface = *itMap ? *itMap : cache.dataInterface;
            if (dataInterface)
            {
                dataInterface->scanAddressesByName(
                    longestWord,
                    StringMatcherMode::CHECK_EQUALS_FROM_SPACE,
                    nullptr,
                    &cache.bbox31,
                    ObfAddressStreetGroupTypesMask().set(ObfAddressStreetGroupType::CityOrTown),
                    true,
                    true,
                    [&found]
                    (const std::shared_ptr<const Address>& address) -> bool
                    {
                        if (address->addressType == AddressType::Street)
                            found << std::static_pointer_cast<const Street>(address);
                        return false;
                    },
                    queryController);
            }
            if (queryController && queryController->isAborted())
                return res;
            itStreets = cache.streets.insert(key, found);
        }

        // the same box test the name index does, on the zoom 16 tile of the street
        const auto roadBBox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(DISTANCE_STREET_NAME_PROXIMITY_BY_NAME,
            Utilities::convertLatLonTo31(*road->connectionPoint));
        for (const auto& street : constOf(*itStreets))
        {
            const PointI tile16((street->position31.x >> 15) << 15, (street->position31.y >> 15) << 15);
            if (roadBBox31.contains(tile16))
                addStreet(road, street, addCommonWords, streetsList, cache);
        }
    }

    if (streetsList.isEmpty())
    {
        res.append(road);
    }
    else
    {
        sortByDistance(streetsList);
        double streetDistance = 0;
        bool isBuildingFound = knownMinBuildingDistance > 0;
        for (const auto& street : constOf(streetsList))
        {
            if (queryController && queryController->isAborted())
                break;
            if (streetDistance == 0)
                streetDistance = street->getDistance();
            else if (isBuildingFound && street->getDistance() > streetDistance + DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME)
                continue;

            street->resetDistance();
            street->connectionPoint = road->connectionPoint;
            auto streetBuildings = loadStreetBuildings(road, street, cache, queryController);
            sortByDistance(streetBuildings);
            if (!streetBuildings.isEmpty())
            {
                auto it = streetBuildings.cbegin();
                if (knownMinBuildingDistance == 0)
                {
                    const auto& firstBld = *it++;
                    knownMinBuildingDistance = firstBld->getDistance();
                    isBuildingFound = true;
                    res.append(firstBld);
                }
                for (; it != streetBuildings.cend(); ++it)
                {
                    if ((*it)->getDistance() > knownMinBuildingDistance * THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER)
                        break;
                    res.append(*it);
                }
            }
            res.append(street);
        }
    }
    sortByDistance(res);
    res.erase(std::remove_if(res.begin(), res.end(),
                             [](const std::shared_ptr<const ResultEntry>& entry)
                             {
                                 return !entry->building && !entry->street &&
                                        !entry->streetGroup && (!entry->road || !entry->road->hasGeocodingAccess());
                             }), res.end());
    return res;
}

bool OsmAnd::ReverseGeocoder_P::addStreet(
        const std::shared_ptr<const ResultEntry>& road,
        const std::shared_ptr<const Street>& street,
        bool matchWithCommonWords,
        QVector<std::shared_ptr<ResultEntry>>& streetsList,
        StreetsCache& cache) const
{
    // "<Segré>" holds the houses of a place without a street, it is not a street named after the place
    if (street->nativeName.startsWith(QLatin1Char('<')))
        return false;
    if (!matchStreetName(road->streetName, street->nativeName, matchWithCommonWords, cache.words))
        return false;
    const auto streetLocation = Utilities::convert31ToLatLon(street->position31);
    const double d = Utilities::distance(streetLocation, *road->searchPoint);
    // double check to support old format
    if (d >= DISTANCE_STREET_NAME_PROXIMITY_BY_NAME)
        return false;
    const auto rs = std::make_shared<ResultEntry>();
    rs->road = road->road;
    rs->streetName = road->streetName;
    rs->point = road->point;
    rs->searchPoint = road->searchPoint;
    rs->street = street;
    rs->streetGroup = street->streetGroup;
    // set connection point to sort
    rs->connectionPoint = streetLocation;
    rs->setDistance(d);
    streetsList.append(rs);
    return true;
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::loadStreetBuildings(
        const std::shared_ptr<const ResultEntry>& road,
        const std::shared_ptr<const ResultEntry>& street,
        StreetsCache& cache,
        const std::shared_ptr<const IQueryController>& queryController) const
{
    QVector<std::shared_ptr<const ResultEntry>> streetBuildings;
    auto itBuildings = cache.buildings.find(street->street.get());
    if (itBuildings == cache.buildings.end())
    {
        QHash<std::shared_ptr<const Street>, QList<std::shared_ptr<const Building>>> buildingsForStreet;
        if (cache.dataInterface)
            cache.dataInterface->loadBuildingsFromStreets({ street->street }, &buildingsForStreet, nullptr, nullptr, queryController);
        itBuildings = cache.buildings.insert(street->street.get(), buildingsForStreet.value(street->street));
    }
    const auto makeResult =
        [&street, &streetBuildings]
        (const std::shared_ptr<const Building>& b, const LatLon& connectionPoint)
        {
            const auto bld = std::make_shared<ResultEntry>();
            bld->road = street->road;
            bld->streetName = street->streetName;
            bld->point = street->point;
            bld->searchPoint = street->searchPoint;
            bld->street = street->street;
            bld->streetGroup = street->streetGroup;
            bld->building = b;
            bld->connectionPoint = connectionPoint;
            streetBuildings.append(bld);
            return bld;
        };
    for (const auto& b : constOf(*itBuildings))
    {
        if (b->interpolation != Building::Interpolation::Disabled)
        {
            const LatLon s = Utilities::convert31ToLatLon(b->position31);
            const LatLon to = Utilities::convert31ToLatLon(b->interpolationPosition31);
            const double coeff = Utilities::projectionCoeff31(*road->searchPoint31(), b->position31, b->interpolationPosition31);
            const double plat = s.latitude + (to.latitude - s.latitude) * coeff;
            const double plon = s.longitude + (to.longitude - s.longitude) * coeff;
            if (Utilities::distance(road->searchPoint->longitude, road->searchPoint->latitude, plon, plat) < DISTANCE_BUILDING_PROXIMITY)
            {
                const auto bld = makeResult(b, LatLon(plat, plon));
                const auto nm = b->getInterpolationName(coeff);
                if (!nm.isEmpty())
                    bld->buildingInterpolation = nm;
            }
        }
        else
        {
            const auto location = Utilities::convert31ToLatLon(b->position31);
            if (Utilities::distance(location, *road->searchPoint) < DISTANCE_BUILDING_PROXIMITY)
                makeResult(b, location);
        }
    }
    return streetBuildings;
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::reverseGeocodingSearch(
        const LatLon searchPoint) const
{
    QVector<std::shared_ptr<const ResultEntry>> result{};
    auto searchPoint31 = Utilities::convertLatLonTo31(searchPoint);
    const auto roadFilter =
        [](const std::shared_ptr<const OsmAnd::Road>& road) -> bool
        {
            const bool isTransportStop =
                road->containsAttribute(QStringLiteral("railway"), QStringLiteral("platform")) ||
                road->containsAttribute(QStringLiteral("public_transport"), QStringLiteral("platform"));
            return !road->captions.isEmpty() && !isTransportStop;
        };
    auto roads = roadLocator->findNearestRoads(searchPoint31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * 2,
                                               OsmAnd::RoutingDataLevel::Detailed, roadFilter);
    if (roads.isEmpty())
        roads = roadLocator->findNearestRoads(searchPoint31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * 10,
                                              OsmAnd::RoutingDataLevel::Detailed, roadFilter);

    double distSquare = 0;
    // we allow duplications to search in both files for boundary regions
    QSet<QPair<ObfObjectId, const ObfInfo*>> set{};
    QSet<QPair<QString, const ObfInfo*>> streetNames{};
    for (auto p : roads)
    {
        auto road = p.first;
        const auto roadObf = obfOf(road);
        double roadDistSquare = p.second->distSquare;
        if (set.contains(qMakePair(road->id, roadObf)))
            continue;
        else
            set.insert(qMakePair(road->id, roadObf));
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
            entry->point = p.second;
            entry->searchPoint = searchPoint;
            entry->connectionPoint = LatLon(Utilities::get31LatitudeY(p.second->preciseY), Utilities::get31LongitudeX(p.second->preciseX));

            if (!streetNames.contains(qMakePair(entry->streetName, roadObf)))
            {
                streetNames.insert(qMakePair(entry->streetName, roadObf));
                result.append(entry);
            }
        }
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS, 2) &&
                distSquare != 0 && roadDistSquare > THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER * distSquare)
            break;
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2))
            break;
    }
    sortByDistance(result);
    return result;
}

// filter duplicate city results (when building is in both regions on boundary)
void OsmAnd::ReverseGeocoder_P::filterDuplicateRegionResults(QVector<std::shared_ptr<const ResultEntry>>& res)
{
    sortByDistance(res);
    // the same street, or the same house number on it, from two maps or from two cities of one map;
    // the street name too (not only the name of the road it was found for), the nearer one on equal cities
    const auto cmpResult =
        [](const std::shared_ptr<const ResultEntry>& gr1, const std::shared_ptr<const ResultEntry>& gr2) -> int
        {
            if (gr1->streetName != gr2->streetName || !gr1->street || !gr2->street || !gr1->streetGroup || !gr2->streetGroup)
                return 0;
            if (gr1->street->nativeName != gr2->street->nativeName)
                return 0;
            const auto buildingName =
                [](const ResultEntry& r)
                {
                    return !r.buildingInterpolation.isEmpty() ? r.buildingInterpolation : r.building->nativeName;
                };
            if (gr1->building && gr2->building)
            {
                if (buildingName(*gr1) != buildingName(*gr2))
                    return 0;
            }
            else if (gr1->building || gr2->building)
            {
                return 0;
            }
            return gr1->getCityDistance() <= gr2->getCityDistance() ? -1 : 1;
        };
    for (int i = 0; i < res.size() - 1;)
    {
        const int cmp = cmpResult(res[i], res[i + 1]);
        if (cmp > 0)
            res.removeAt(i);
        else if (cmp < 0)
            res.removeAt(i + 1);
        else
            i++; // nothing to delete
    }
}

// the addresses of the roads found by reverseGeocodingSearch: the street and the buildings of
// every road, duplicates from neighbouring maps and buildings far behind the nearest dropped,
// nearest first; stops early when the query is aborted
QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::findAddresses(
        const QVector<std::shared_ptr<const ResultEntry>>& roads,
        const std::shared_ptr<const IQueryController>& queryController) const
{
    QVector<std::shared_ptr<const ResultEntry>> complete;
    if (roads.isEmpty())
        return complete;
    double minBuildingDistance = 0;
    StreetsCache cache;
    const auto searchPoint31 = *roads.first()->searchPoint31();
    cache.bbox31 = (AreaI)Utilities::boundingBox31FromAreaInMeters(
        DISTANCE_STREET_NAME_PROXIMITY_BY_NAME + StreetsCache::MARGIN, searchPoint31);
    cache.dataInterface = owner->obfsCollection->obtainDataInterface(
        &cache.bbox31, MinZoomLevel, MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::Address));
    for (const auto& r : constOf(roads))
    {
        if (queryController && queryController->isAborted())
            break;
        const auto streetAndBuildings = findStreetAndBuildings(r, minBuildingDistance, cache, queryController);
        if (!streetAndBuildings.isEmpty())
        {
            const double md = streetAndBuildings.first()->getDistance();
            minBuildingDistance = (minBuildingDistance == 0) ? md : std::min(md, minBuildingDistance);
            streetAndBuildings.first()->setDistance(-1); // clear intermediate cached distance
            complete.append(streetAndBuildings);
        }
    }
    filterDuplicateRegionResults(complete);
    complete.erase(std::remove_if(complete.begin(), complete.end(),
                                  [minBuildingDistance](const std::shared_ptr<const ResultEntry>& r)
                                  {
                                      return r->building &&
                                             r->getDistance() > minBuildingDistance * THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER;
                                  }), complete.end());
    sortByDistance(complete);
    return complete;
}
