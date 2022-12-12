#include "IFavoriteLocation.h"

OsmAnd::IFavoriteLocation::IFavoriteLocation()
{
}

OsmAnd::IFavoriteLocation::~IFavoriteLocation()
{
}

OsmAnd::LatLon OsmAnd::IFavoriteLocation::getLatLonPrecised() const
{
    const auto& latLon = getLatLon();
    int coef = qPow(10, kFavoriteLatLonPrecision);
    int lat = (int)(latLon.latitude * coef + .5);
    int lon = (int)(latLon.longitude * coef + .5);
    return LatLon((double)lat / coef, (double)lon / coef);
}

uint32_t OsmAnd::IFavoriteLocation::hash() const
{
    return qHash(getPosition31().x) +
            qHash(getPosition31().y) +
            qHash(getTitle()) +
            qHash(getGroup()) +
            qHash(getDescription()) +
            qHash(getColor().toString()) +
            //qHash(getElevation()) +
            qHash(getTime()) +
            qHash(getPickupTime()) +
            qHash(getCalendarEvent()) +
            qHash(getAddress()) +
            qHash(getIcon()) +
            qHash(getBackground()) +
            qHash(getComment()) +
            (isHidden() ? 0 : 1);
}

bool OsmAnd::IFavoriteLocation::isEqual(IFavoriteLocation* favoriteLocation) const
{
    if (favoriteLocation == NULL)
        return false;
    
    return getLatLonPrecised() == favoriteLocation->getLatLonPrecised() &&
            getTitle() == favoriteLocation->getTitle() &&
            getDescription() == favoriteLocation->getDescription() &&
            getGroup() == favoriteLocation->getGroup() &&
            getColor() == favoriteLocation->getColor() &&
            //getElevation() == favoriteLocation->getElevation() &&
            getTime() == favoriteLocation->getTime() &&
            getPickupTime() == favoriteLocation->getPickupTime() &&
            getCalendarEvent() == favoriteLocation->getCalendarEvent() &&
            getAddress() == favoriteLocation->getAddress() &&
            getIcon() == favoriteLocation->getIcon() &&
            getBackground() == favoriteLocation->getBackground() &&
            getComment() == favoriteLocation->getComment() &&
            isHidden() == favoriteLocation->isHidden();
}
