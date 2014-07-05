#include "FavoriteLocation.h"
#include "FavoriteLocation_P.h"

OsmAnd::FavoriteLocation::FavoriteLocation(
	const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink_,
	const PointI position_,
	const QString& title_,
	const QString& group_,
	const ColorRGB color_)
	: _p(new FavoriteLocation_P(this))
    , position(position_)
{
	setTitle(title_);
    setGroup(group_);
    setColor(color_);
	attach(containerLink_);
}

OsmAnd::FavoriteLocation::FavoriteLocation(const PointI position_)
	: _p(new FavoriteLocation_P(this))
	, position(position_)
{
}

OsmAnd::FavoriteLocation::~FavoriteLocation()
{
}

OsmAnd::PointI OsmAnd::FavoriteLocation::getPosition() const
{
    return position;
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
