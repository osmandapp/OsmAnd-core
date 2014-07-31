#include "FavoriteLocation.h"
#include "FavoriteLocation_P.h"

#include "Utilities.h"

OsmAnd::FavoriteLocation::FavoriteLocation(
	const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink_,
	const PointI position31_,
	const QString& title_,
	const QString& group_,
	const ColorRGB color_)
	: _p(new FavoriteLocation_P(this))
    , locationSource(LocationSource::Point31)
    , position31(position31_)
    , latLon(Utilities::convert31ToLatLon(position31_))
{
	setTitle(title_);
    setGroup(group_);
    setColor(color_);
	attach(containerLink_);
}

OsmAnd::FavoriteLocation::FavoriteLocation(
    const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink_,
    const LatLon latLon_,
    const QString& title_,
    const QString& group_,
    const ColorRGB color_)
    : _p(new FavoriteLocation_P(this))
    , locationSource(LocationSource::LatLon)
    , position31(Utilities::convertLatLonTo31(latLon_))
    , latLon(latLon_)
{
    setTitle(title_);
    setGroup(group_);
    setColor(color_);
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

QString OsmAnd::FavoriteLocation::getTitle() const
{
    return _p->getTitle();
}

void OsmAnd::FavoriteLocation::setTitle(const QString& newTitle)
{
    _p->setTitle(newTitle);
}

QString OsmAnd::FavoriteLocation::getGroup() const
{
    return _p->getGroup();
}

void OsmAnd::FavoriteLocation::setGroup(const QString& newGroup)
{
    _p->setGroup(newGroup);
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
