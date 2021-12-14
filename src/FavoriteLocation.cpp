#include "FavoriteLocation.h"
#include "FavoriteLocation_P.h"

#include "Utilities.h"

OsmAnd::FavoriteLocation::FavoriteLocation(
	const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink_,
	const PointI position31_,
    const QString& elevation_,
    const QString& time_,
    const QString& creationTime_,
	const QString& title_,
    const QString& description_,
    const QString& address_,
	const QString& group_,
    const QString& icon_,
    const QString& background_,
    const ColorRGB color_,
    const bool calendarEvent_)
	: _p(new FavoriteLocation_P(this))
    , locationSource(LocationSource::Point31)
    , position31(position31_)
    , latLon(Utilities::convert31ToLatLon(position31_))
{
    setElevation(elevation_);
    setTime(time_);
    setCreationTime(creationTime_);
	setTitle(title_);
    setDescription(description_);
    setAddress(address_);
    setGroup(group_);
    setIcon(icon_);
    setBackground(background_);
    setColor(color_);
    setCalendarEvent(calendarEvent_);
	attach(containerLink_);
}

OsmAnd::FavoriteLocation::FavoriteLocation(
    const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink_,
    const LatLon latLon_,
    const QString& elevation_,
    const QString& time_,
    const QString& creationTime_,
    const QString& title_,
    const QString& description_,
    const QString& address_,
    const QString& group_,
    const QString& icon_,
    const QString& background_,
    const ColorRGB color_,
    const bool calendarEvent_)
    : _p(new FavoriteLocation_P(this))
    , locationSource(LocationSource::LatLon)
    , position31(Utilities::convertLatLonTo31(latLon_))
    , latLon(latLon_)
{
    setElevation(elevation_);
    setTime(time_);
    setCreationTime(creationTime_);
    setTitle(title_);
    setDescription(description_);
    setAddress(address_);
    setGroup(group_);
    setIcon(icon_);
    setBackground(background_);
    setColor(color_);
    setCalendarEvent(calendarEvent_);
    attach(containerLink_);
}

OsmAnd::FavoriteLocation::FavoriteLocation(const PointI position31_)
	: _p(new FavoriteLocation_P(this))
    , locationSource(LocationSource::Point31)
	, position31(position31_)
    , latLon(Utilities::convert31ToLatLon(position31_))
{
}

OsmAnd::FavoriteLocation::FavoriteLocation(const LatLon latLon_)
    : _p(new FavoriteLocation_P(this))
    , locationSource(LocationSource::LatLon)
    , position31(Utilities::convertLatLonTo31(latLon_))
    , latLon(latLon_)
{

}

OsmAnd::FavoriteLocation::~FavoriteLocation()
{
}

OsmAnd::FavoriteLocation::LocationSource OsmAnd::FavoriteLocation::getLocationSource() const
{
    return locationSource;
}

OsmAnd::PointI OsmAnd::FavoriteLocation::getPosition31() const
{
    return position31;
}

OsmAnd::LatLon OsmAnd::FavoriteLocation::getLatLon() const
{
    return latLon;
}

bool OsmAnd::FavoriteLocation::isHidden() const
{
    return _p->isHidden();
}

void OsmAnd::FavoriteLocation::setIsHidden(const bool isHidden)
{
    _p->setIsHidden(isHidden);
}

QString OsmAnd::FavoriteLocation::getElevation() const
{
    return _p->getElevation();
}

void OsmAnd::FavoriteLocation::setElevation(const QString& newElevation)
{
    _p->setElevation(newElevation);
}

QString OsmAnd::FavoriteLocation::getTime() const
{
    return _p->getTime();
}

void OsmAnd::FavoriteLocation::setTime(const QString& newTime)
{
    _p->setTime(newTime);
}

QString OsmAnd::FavoriteLocation::getCreationTime() const
{
    return _p->getCreationTime();
}

void OsmAnd::FavoriteLocation::setCreationTime(const QString& newTime)
{
    _p->setCreationTime(newTime);
}

void OsmAnd::FavoriteLocation::setCalendarEvent(const bool calendarEvent)
{
    _p->setCalendarEvent(calendarEvent);
}

bool OsmAnd::FavoriteLocation::getCalendarEvent() const
{
    return _p->getCalendarEvent();
}

QString OsmAnd::FavoriteLocation::getTitle() const
{
    return _p->getTitle();
}

void OsmAnd::FavoriteLocation::setTitle(const QString& newTitle)
{
    _p->setTitle(newTitle);
}

QString OsmAnd::FavoriteLocation::getDescription() const
{
    return _p->getDescription();
}

void OsmAnd::FavoriteLocation::setDescription(const QString& newDescription)
{
    _p->setDescription(newDescription);
}

QString OsmAnd::FavoriteLocation::getAddress() const
{
    return _p->getAddress();
}

void OsmAnd::FavoriteLocation::setAddress(const QString& newAddress)
{
    _p->setAddress(newAddress);
}

QString OsmAnd::FavoriteLocation::getGroup() const
{
    return _p->getGroup();
}

void OsmAnd::FavoriteLocation::setGroup(const QString& newGroup)
{
    _p->setGroup(newGroup);
}

QString OsmAnd::FavoriteLocation::getIcon() const
{
    return _p->getIcon();
}

void OsmAnd::FavoriteLocation::setIcon(const QString& newIcon)
{
    _p->setIcon(newIcon);
}

QString OsmAnd::FavoriteLocation::getBackground() const
{
    return _p->getBackground();
}

void OsmAnd::FavoriteLocation::setBackground(const QString& newBackground)
{
    
    _p->setBackground(newBackground);
}

OsmAnd::ColorRGB OsmAnd::FavoriteLocation::getColor() const
{
    return _p->getColor();
}

void OsmAnd::FavoriteLocation::setColor(const ColorRGB newColor)
{
    _p->setColor(newColor);
}

void OsmAnd::FavoriteLocation::attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink)
{
	_p->attach(containerLink);
}

void OsmAnd::FavoriteLocation::detach()
{
	_p->detach();
}
