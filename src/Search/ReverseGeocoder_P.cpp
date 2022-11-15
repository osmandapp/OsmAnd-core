#include "ReverseGeocoder_P.h"

#include "AddressesByNameSearch.h"
#include "Building.h"
#include "Logging.h"
#include "ObfDataInterface.h"
#include "Road.h"
#include "Utilities.h"

#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Search/CommonWords.h>
#include <routePlannerFrontEnd.h>
#include <routingContext.h>
#include <QStringBuilder>

//
//  OsmAnd-java/src/net/osmand/binary/GeocodingUtilities.java
//  git revision 0699ae019e3929ac68bd9142b90ec6b797996400
//

// Location to test parameters http://www.openstreetmap.org/#map=18/53.896473/27.540071 (hno 44)
const float THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER = 5;
const float STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS = 250;
const float STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS = 400;

const float DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME = 1000;
const float DISTANCE_STREET_NAME_PROXIMITY_BY_NAME = 45000;

const float THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER = 1.5f;
const float DISTANCE_BUILDING_PROXIMITY = 100;
const float  GPS_POSSIBLE_ERROR = 7;

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
    const std::shared_ptr<RoutingContext>& ctx,
    const ISearch::NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);
    if (!criteria.latLon.isSet() && !criteria.position31.isSet())
        return;
    auto searchPoint = criteria.latLon.isSet() ? *criteria.latLon : Utilities::convert31ToLatLon(*criteria.position31);
    QVector<std::shared_ptr<const ResultEntry>> roads = reverseGeocodeToRoads(searchPoint, ctx, false);
    std::shared_ptr<const ResultEntry> result = justifyResult(roads);
    newResultEntryCallback(criteria, *result);
}

bool OsmAnd::ReverseGeocoder_P::DISTANCE_COMPARATOR(
        const std::shared_ptr<const ResultEntry>& a,
        const std::shared_ptr<const ResultEntry>& b)
{
    return Utilities::distance(a->connectionPoint, a->searchPoint) < Utilities::distance(b->connectionPoint, b->searchPoint);
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
                        // double check to suport old format
                        if (d < DISTANCE_STREET_NAME_PROXIMITY_BY_NAME) {
                            const std::shared_ptr<ResultEntry> rs = std::make_shared<ResultEntry>();
                            rs->road = road->road;
                            rs->street = street;
                            rs->point = road->point;
                            rs->searchPoint = road->searchPoint;
                            // set connection point to sort
                            rs->connectionPoint = Utilities::convert31ToLatLon(street->position31);
                            rs->streetGroup = street->streetGroup;
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
        bool isBuildingFound = knownMinBuildingDistance > 0;
        for (const std::shared_ptr<ResultEntry> street : streetList)
        {
            if (streetDistance == 0)
                streetDistance = street->getDistance();
            else if (streetDistance > 0 && street->getDistance() > streetDistance + DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME && isBuildingFound)
                continue;
            
            street->connectionPoint = road->connectionPoint;
            QVector<std::shared_ptr<const ResultEntry>> streetBuildings = loadStreetBuildings(road, street);
            std::sort(streetBuildings.begin(), streetBuildings.end(), DISTANCE_COMPARATOR);
            if (!streetBuildings.isEmpty())
            {
                if (knownMinBuildingDistance == 0)
                {
                    knownMinBuildingDistance = streetBuildings[0]->getDistance();
                    isBuildingFound = true;
                }

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
    for (const std::shared_ptr<const Building> b : buildings)
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

std::shared_ptr<RouteSegmentPoint> OsmAnd::ReverseGeocoder_P::findRouteSegment(
        const LatLon searchPoint, const std::shared_ptr<RoutingContext>& ctx, std::vector<std::shared_ptr<RouteSegmentPoint>>& list) const
{
    auto searchPoint31 = Utilities::convertLatLonTo31(searchPoint);
    auto dataObjects = roadLocator->findNearestRoads(searchPoint31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * 2, OsmAnd::RoutingDataLevel::Detailed,
                                               [this]
                                               (const std::shared_ptr<const OsmAnd::Road>& road) -> bool
                                               {
                                                   return !road->captions.isEmpty();
                                               });
    if (dataObjects.isEmpty())
        dataObjects = roadLocator->findNearestRoads(searchPoint31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * 10, OsmAnd::RoutingDataLevel::Detailed,
                                              [this]
                                              (const std::shared_ptr<const OsmAnd::Road>& road) -> bool
                                              {
                                                  return !road->captions.isEmpty();
                                              });
            
    if (list.empty())
        list = {};

    for (auto r : dataObjects)
    {
        if (r.first->points31.size() > 0)
        {
            std::shared_ptr<RouteSegmentPoint> road = nullptr;
            for (int j = 1; j < r.first->points31.size(); j++)
            {
                std::pair<int, int> pr = Utilities::getProjectionPoint31(searchPoint31.x, searchPoint31.y, r.first->points31[j-1].x, r.first->points31[j-1].y, r.first->points31[j].x, r.first->points31[j].y);
                double currentsDistSquare = Utilities::squareDistance31(pr.first, pr.second, searchPoint31.x, searchPoint31.y);
                if (road == nullptr || currentsDistSquare < road->dist)
                {
                    SHARED_PTR<RouteDataObject> ro = std::make_shared<RouteDataObject>();
                    std::vector<uint32_t> pointsX = {};
                    std::vector<uint32_t> pointsY = {};
                    for (int k = 0; k < r.first->points31.size(); k++)
                    {
                        pointsX.push_back(r.first->points31[k].x);
                        pointsY.push_back(r.first->points31[k].y);
                    }
                    ro->pointsX = pointsX;
                    ro->pointsY = pointsX;
                    for (uint32_t key : r.first->captions.keys())
                    {
                        ro->names[(int)key] = r.first->captions[key].toStdString();
                    }
                    
                    road = std::make_shared<RouteSegmentPoint>(ro, j, sqrt(currentsDistSquare));
                    road->preciseX = pr.first;
                    road->preciseY = pr.second;
                    this->regionsCache[road] = r.first->section->name;
                }
            }
            
            if (road != nullptr)
            {
                float prio = ctx->config->router->defineDestinationPriority(road->road);
                if (prio > 0)
                {
                    road->dist = sqrt((road->dist * road->dist + GPS_POSSIBLE_ERROR * GPS_POSSIBLE_ERROR)
                    / (prio * prio));
                    list.push_back(road);
                }
            }
        }
    }
     
    std::sort(list.begin(), list.end(), [](std::shared_ptr<RouteSegmentPoint> o1, std::shared_ptr<RouteSegmentPoint> o2) {
            return o1->dist < o2->dist;
        });

    if (list.size() > 0)
    {
        std::shared_ptr<RouteSegmentPoint> ps = nullptr;
        //if (ctx->publicTransport)
        //{
        //}
        if (ps == nullptr)
        {
            ps = list[0];
        }
        ps->others = list;
        return ps;
    }
    return nullptr;
}

QVector<std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry>> OsmAnd::ReverseGeocoder_P::reverseGeocodeToRoads(
        const LatLon searchPoint, const std::shared_ptr<RoutingContext>& ctx, const bool allowEmptyNames) const
{
    QVector<std::shared_ptr<const ResultEntry>> lst{};
    std::vector<std::shared_ptr<RouteSegmentPoint>> listR = {};
    this->regionsCache = {};
    // we allow duplications to search in both files for boundary regions
    // here we use same code as for normal routing, so we take into account current profile and sort by priority & distance
    findRouteSegment(searchPoint, ctx, listR);
    double distSquare = 0;
    QHash<QString, QList<QString>> streetNames{};
    
    for (std::shared_ptr<RouteSegmentPoint> p : listR)
    {
        std::shared_ptr<RouteDataObject>& road = p->getRoad();
        double roadDistSquare = std::pow(p->dist, 2);
        string name;
        if (road->getName().empty())
        {
            std::string langEmptyString = std::string();
            name = road->getRef(langEmptyString, false, true);
        }
        else
        {
            name = road->getName();
        }
        
        if (allowEmptyNames || !name.empty())
        {
            if (distSquare == 0 || distSquare > roadDistSquare)
            {
                distSquare = roadDistSquare;
            }
            
            std::shared_ptr<ResultEntry> sr = std::make_shared<ResultEntry>();
            sr->searchPoint = searchPoint;
            sr->streetName = name.empty() ? QStringLiteral() : QString::fromUtf8(name.c_str());
            sr->point = p;
            sr->connectionPoint = LatLon(Utilities::get31LatitudeY(p->preciseY), Utilities::get31LongitudeX(p->preciseX));
            //sr->regionFP = road->region->filePointer;
            //sr->regionLen = road->region->length;
            
            QList<QString> plst = streetNames[sr->streetName];
            if (plst.empty())
            {
                streetNames[sr->streetName] = plst;
            }
            
            QString regionName = this->regionsCache[p];
            if (!plst.contains(regionName))
            {
                plst.append(regionName);
                streetNames[sr->streetName] = plst;
                lst.push_back(sr);
            }
        }
        
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS, 2) &&
                distSquare != 0 && roadDistSquare > THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER * distSquare)
            break;
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2))
            break;
    }
    
    std::sort(lst.begin(), lst.end(), DISTANCE_COMPARATOR);
    return lst;
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
    
    OsmAnd::ReverseGeocoder_P::filterDuplicateRegionResults(complete);
    std::sort(complete.begin(), complete.end(), DISTANCE_COMPARATOR);
    //utilities.sortGeocodingResults(readers, complete);
    
    return !complete.isEmpty() ? complete[0] : std::make_shared<ResultEntry>();
}

void OsmAnd::ReverseGeocoder_P::filterDuplicateRegionResults(QVector<std::shared_ptr<const ResultEntry>>& res) const
{
    std::sort(res.begin(), res.end(), DISTANCE_COMPARATOR);
    // filter duplicate city results (when building is in both regions on boundary)
    for (int i = 0; i < res.size() - 1;)
    {
        int cmp = cmpResult(res[i], res[i+1]);
        if (cmp > 0)
        {
            res.remove(i);
        }
        else if (cmp < 0)
        {
            res.remove(i+1);
        }
        else
        {
            // nothing to delete
            i++;
        }
    }
}

int OsmAnd::ReverseGeocoder_P::cmpResult(std::shared_ptr<const ResultEntry> gr1, std::shared_ptr<const ResultEntry> gr2) const
{
    QString name1 = gr1->streetName.isEmpty() ? gr1->street->getName("", false) : gr1->streetName;
    QString name2 = gr2->streetName.isEmpty() ? gr2->street->getName("", false) : gr2->streetName;
    bool eqStreet= name1.compare(name2) == 0;
    
    if (eqStreet)
    {
        bool sameObj = false;
        if (gr1->streetGroup != nullptr && gr2->streetGroup != nullptr)
        {
            if (gr1->building != nullptr && gr2->building != nullptr)
            {
                QString gr1BuildingName = gr1->building->getName("", false);
                QString gr2BuildingName = gr2->building->getName("", false);
                if (gr1BuildingName.compare(gr2BuildingName) == 0)
                {
                    // same building
                    sameObj = true;
                }
            }
            else if (gr1->building == nullptr && gr2->building == nullptr)
            {
                // same street
                sameObj = true;
            }
        }
        if (sameObj)
        {
            double cityDist1 = Utilities::distance31(gr1->searchPoint31()->x, gr1->searchPoint31()->y, gr1->streetGroup->position31.x, gr1->streetGroup->position31.y);
            double cityDist2 = Utilities::distance31(gr2->searchPoint31()->x, gr2->searchPoint31()->y, gr2->streetGroup->position31.x, gr2->streetGroup->position31.y);
            if (cityDist1 < cityDist2) {
                return -1;
            } else {
                return 1;
            }
        }
    }
    return 0;
}
