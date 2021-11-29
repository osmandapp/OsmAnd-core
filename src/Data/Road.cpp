#include "Road.h"

#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"
#include "QKeyValueIterator.h"

#include <ICU.h>
#include <OsmAndCore/Utilities.h>

const QHash<QString, QStringList> GEOCODING_ACCESS {
    {
        QLatin1String("highway"),
        {
            QLatin1String("motorway"),
            QLatin1String("motorway_link"),
            QLatin1String("trunk"),
            QLatin1String("trunk_link"),
            QLatin1String("primary"),
            QLatin1String("primary_link"),
            QLatin1String("secondary"),
            QLatin1String("secondary_link"),
            QLatin1String("tertiary"),
            QLatin1String("tertiary_link"),
            QLatin1String("unclassified"),
            QLatin1String("road"),
            QLatin1String("residential"),
            QLatin1String("track"),
            QLatin1String("service"),
            QLatin1String("living_street"),
            QLatin1String("pedestrian")
        }
    },
    {
        QLatin1String("route"),
        {
            QLatin1String("ferry"),
            QLatin1String("shuttle_train")
        }
    }
};

OsmAnd::Road::Road(const std::shared_ptr<const ObfRoutingSectionInfo>& section_)
    : ObfMapObject(section_)
    , section(section_)
{
    attributeMapping = section->getAttributeMapping();
}

const bool OsmAnd::Road::hasGeocodingAccess() const
{
    bool access = false;
    for (const auto& entry : rangeOf(constOf(GEOCODING_ACCESS)))
    {
        QString key = entry.key();
        for (const auto& v : constOf(entry.value()))
        {
            if (this->containsAttribute(key, v))
            {
                access = true;
                break;
            }
            
        }
        if (access)
            break;
    }
    return access;
}

QString OsmAnd::Road::getRefInNativeLanguage() const
{
    const auto citName = captions.constFind(section->getAttributeMapping()->refAttributeId);
    if (citName == captions.cend())
        return {};
    return *citName;
}

QString OsmAnd::Road::getRefInLanguage(const QString& lang) const
{
    const auto citNameAttributeId = section->getAttributeMapping()->localizedRefAttributes.constFind(&lang);
    if (citNameAttributeId == section->getAttributeMapping()->localizedRefAttributes.cend())
        return {};
    
    const auto citCaption = captions.constFind(*citNameAttributeId);
    if (citCaption == captions.cend())
        return {};
    return *citCaption;
}

QString OsmAnd::Road::getRef(const QString lang, bool transliterate) const
{
    QString name = QString();
    if (!lang.isEmpty())
        name = getRefInLanguage(lang);
    
    if (name.isNull())
        name = getRefInNativeLanguage();
    
    if (transliterate && !name.isNull())
        return OsmAnd::ICU::transliterateToLatin(name);
    else
        return name;
}

QString OsmAnd::Road::getDestinationRef(bool direction) const
{
    if (!captions.empty())
    {
        const auto& kt = captions.keys();
        QString refTag = (direction == true) ? QLatin1String("destination:ref:forward") : QLatin1String("destination:ref:backward");
        QString refTagDefault = QLatin1String("destination:ref");
        QString refDefault = QString();
        
        for(int i = 0 ; i < kt.size(); i++)
        {
            auto k = kt[i];
            if (section->getAttributeMapping()->decodeMap.size() > k)
            {
                if (refTag == section->getAttributeMapping()->decodeMap[k].tag)
                    return captions[k];
                
                if (refTagDefault == section->getAttributeMapping()->decodeMap[k].tag)
                    refDefault = captions[k];
            }
        }
        if (!refDefault.isNull())
            return refDefault;

        //return names.get(region.refTypeRule);
    }
    return QString();
}

QString OsmAnd::Road::getDestinationName(const QString lang, bool transliterate, bool direction) const
{
    //Issue #3289: Treat destination:ref like a destination, not like a ref
    QString destRef = (getDestinationRef(direction).isNull() || getDestinationRef(direction) == getRef(lang, transliterate)) ? QString() : getDestinationRef(direction);
    QString destRef1 = destRef.isEmpty() ? QString() : destRef + QLatin1String(", ");
    
    if (!captions.empty())
    {
        const auto& kt = captions.keys();

        // Issue #3181: Parse destination keys in this order:
        //              destination:lang:XX:forward/backward
        //              destination:forward/backward
        //              destination:lang:XX
        //              destination
        
        QString destinationTagLangFB = QLatin1String("destination:lang:XX");
        if(!lang.isEmpty())
        {
            destinationTagLangFB = (direction == true) ? QLatin1String("destination:lang:") + lang + QLatin1String(":forward") : QLatin1String("destination:lang:") + lang + QLatin1String(":backward");
        }
        QString destinationTagFB = (direction == true) ? QLatin1String("destination:forward") : QLatin1String("destination:backward");
        QString destinationTagLang = QLatin1String("destination:lang:XX");
        if (!lang.isEmpty())
            destinationTagLang = QLatin1String("destination:lang:") + lang;
        
        QString destinationTagDefault = QLatin1String("destination");
        QString destinationDefault = QString();
        
        for(int i = 0 ; i < kt.size(); i++)
        {
            auto k = kt[i];
            if (section->getAttributeMapping()->decodeMap.size() > k)
            {
                if (!lang.isEmpty() && destinationTagLangFB == section->getAttributeMapping()->decodeMap[k].tag)
                    return destRef1 + (transliterate ? OsmAnd::ICU::transliterateToLatin(captions[k]) : captions[k]);
                
                if (destinationTagFB == section->getAttributeMapping()->decodeMap[k].tag)
                    return destRef1 + (transliterate ? OsmAnd::ICU::transliterateToLatin(captions[k]) : captions[k]);
            
                if (!lang.isEmpty() && destinationTagLang == section->getAttributeMapping()->decodeMap[k].tag)
                    return destRef1 + (transliterate ? OsmAnd::ICU::transliterateToLatin(captions[k]) : captions[k]);
            
                if (destinationTagDefault == section->getAttributeMapping()->decodeMap[k].tag)
                    destinationDefault = captions[k];
            }
        }
        if (!destinationDefault.isNull())
            return destRef1 + (transliterate ? OsmAnd::ICU::transliterateToLatin(destinationDefault) : destinationDefault);
    }
    return destRef.isEmpty() ? QString() : destRef;
}

float OsmAnd::Road::getMaximumSpeed(bool direction) const
{
    const auto& decodeMap = section->getAttributeMapping()->routingDecodeMap;
    float maxSpeed = 0;
    for (int i = 0; i < attributeIds.size(); i++) {
        const auto r = decodeMap.getRef(attributeIds[i]);
        if (r)
        {
            if (r->isForward() != 0)
            {
                if ((r->isForward() == 1) != direction)
                    continue;
            }
            float mx = r->maxSpeed();
            if (mx > 0)
            {
                maxSpeed = mx;
                // conditional has priority
                if (r->conditional())
                    break;
            }
        }
    }
    return maxSpeed;
}

bool OsmAnd::Road::isDeleted() const
{
    const auto& decodeMap = section->getAttributeMapping()->routingDecodeMap;
    float maxSpeed = 0;
    for (int i = 0; i < attributeIds.size(); i++) {
        const auto r = decodeMap.getRef(attributeIds[i]);
        if (r)
        {
            if (r->getTag() == QStringLiteral("osmand_change") && r->getValue() == QStringLiteral("delete"))
                return true;
        }
    }
    return false;
}

// Gives route direction of EAST degrees from NORTH ]-PI, PI]
double OsmAnd::Road::directionRoute(int startPoint, bool plus, float dist) const
{
    int x = points31[startPoint].x;
    int y = points31[startPoint].y;
    int nx = startPoint;
    int px = x;
    int py = y;
    double total = 0;
    do
    {
        if (plus)
        {
            nx++;
            if (nx >= (int) points31.size())
                break;
        }
        else
        {
            nx--;
            if (nx < 0)
                break;
        }
        px = points31[nx].x;
        py = points31[nx].y;
        // translate into meters
        total += abs(px - x) * 0.011 + abs(py - y) * 0.01863;
    }
    while (total < dist);
    
    return -atan2( (float)x - px, (float) y - py );
}


double OsmAnd::Road::directionRoute(int startPoint, bool plus) const
{
    // same goes to C++
    // Victor : the problem to put more than 5 meters that BinaryRoutePlanner will treat
    // 2 consequent Turn Right as UT and here 2 points will have same turn angle
    // So it should be fix in both places
    return directionRoute(startPoint, plus, 5);
}

bool OsmAnd::Road::bearingVsRouteDirection(double bearing) const
{
    bool direction = true;
    if (bearing >= 0)
    {
        double diff = Utilities::normalizedAngleRadians(directionRoute(0, true) - bearing / 180.f * M_PI);
        direction = fabs(diff) < M_PI / 2.f;
    }
    return direction;
}


//OsmAnd::Road::Road( const std::shared_ptr<const Road>& that, int insertIdx, uint32_t x31, uint32_t y31 )
//    : _ref(that)
//    , _id(0)
//    , _points(_ref->_points.size() + 1)
//    , subsection(_ref->subsection)
//    , id(_ref->_id)
//    , names(_ref->_names)
//    , points(_points)
//    , types(_ref->_types)
//    , pointsTypes(_pointsTypes)
//    , restrictions(_ref->_restrictions)
//{
//    int pointIdx = 0;
//    for(; pointIdx < insertIdx; pointIdx++)
//    {
//        _points[pointIdx] = _ref->_points[pointIdx];
//        if (!_ref->_pointsTypes.isEmpty())
//            _pointsTypes.insert(pointIdx, _ref->_pointsTypes[pointIdx]);
//    }
//    _points[pointIdx++] = PointI(x31, y31);
//    for(const auto count = _points.size(); pointIdx < count; pointIdx++)
//    {
//        _points[pointIdx] = _ref->_points[pointIdx - 1];
//        if (!_ref->_pointsTypes.isEmpty())
//            _pointsTypes.insert(pointIdx, _ref->_pointsTypes[pointIdx]);
//    }
//}

OsmAnd::Road::~Road()
{
}

//double OsmAnd::Road::getDirectionDelta( uint32_t originIdx, bool forward ) const
//{
//    //NOTE: Victor: the problem to put more than 5 meters that BinaryRoutePlanner will treat
//    // 2 consequent Turn Right as UT and here 2 points will have same turn angle
//    // So it should be fix in both places
//    return getDirectionDelta(originIdx, forward, 5);
//}
//
//double OsmAnd::Road::getDirectionDelta( uint32_t originIdx, bool forward, float distance ) const
//{
//    auto itPoint = (_points.cbegin() + originIdx);
//    const auto itOriginPoint = itPoint;
//    float scannedDistance = 0.0;
//    do
//    {
//        if (forward)
//        {
//            itPoint++;
//            if (itPoint == _points.cend())
//            {
//                itPoint--;
//                break;
//            }
//        }
//        else
//        {
//            if (itPoint == _points.cbegin())
//                break;
//            itPoint--;
//        }
//
//        // translate into meters
//        scannedDistance +=
//            Utilities::x31toMeters(qAbs((int64_t)itPoint->x - (int64_t)itOriginPoint->x)) +
//            Utilities::y31toMeters(qAbs((int64_t)itPoint->y - (int64_t)itOriginPoint->y));
//    } while ( scannedDistance < distance );
//
//    return -qAtan2(itOriginPoint->x - itPoint->x, itOriginPoint->y - itPoint->y);
//}
//
//OsmAnd::RoadDirection OsmAnd::Road::getDirection() const
//{
//    const auto& encRules = subsection->section->_p->_encodingRules;
//    for(const auto& type : constOf(types))
//    {
//        const auto& rule = encRules[type];
//
//        if (rule->_type == ObfRoutingSectionInfo_P::EncodingRule::OneWay)
//            return static_cast<RoadDirection>(rule->_parsedValue.asSignedInt);
//        else if (rule->_type == ObfRoutingSectionInfo_P::EncodingRule::Roundabout)
//            return RoadDirection::OneWayForward;
//    }
//    
//    return RoadDirection::TwoWay;
//}
//
//bool OsmAnd::Road::isRoundabout() const
//{
//    const auto& encRules = subsection->section->_p->_encodingRules;
//    for(const auto& type : constOf(types))
//    {
//        const auto& rule = encRules[type];
//
//        if (rule->isRoundabout())
//            return true;
//        else if (rule->getDirection() != RoadDirection::TwoWay && isLoop())
//            return true;
//    }
//
//    return false;
//}
//
//int OsmAnd::Road::getLanes() const
//{
//    const auto& encRules = subsection->section->_p->_encodingRules;
//    for(const auto& type : constOf(types))
//    {
//        const auto& rule = encRules[type];
//
//        if (rule->_type == ObfRoutingSectionInfo_P::EncodingRule::Lanes)
//            return rule->_parsedValue.asUnsignedInt;
//    }
//    return -1;
//}
//
//QString OsmAnd::Road::getHighway() const
//{
//    const auto& encRules = subsection->section->_p->_encodingRules;
//    for(const auto& type : constOf(types))
//    {
//        const auto& rule = encRules[type];
//
//        if (rule->_type == ObfRoutingSectionInfo_P::EncodingRule::Highway)
//            return rule->_value;
//    }
//    return QString();
//}
//
//bool OsmAnd::Road::isLoop() const
//{
//    assert(_points.size() > 0);
//
//    return _points.first() == _points.last();
//}
