#include "FavoriteLocation_P.h"
#include "FavoriteLocation.h"

#include "FavoriteLocationsCollection.h"
#include "FavoriteLocationsCollection_P.h"

OsmAnd::FavoriteLocation_P::FavoriteLocation_P(Link<FavoriteLocationsCollection*>& containerLink, FavoriteLocation* const owner_)
	: _weakLink(containerLink)
	, owner(owner_)
{
}

OsmAnd::FavoriteLocation_P::~FavoriteLocation_P()
{
}

QString OsmAnd::FavoriteLocation_P::getTitle() const
{
	QReadLocker scopedLocker(&_lock);

	return _title;
}

void OsmAnd::FavoriteLocation_P::setTitle(const QString& newTitle)
{
	QWriteLocker scopedLocker(&_lock);

	_title = newTitle;

	if (const auto link = _weakLink.lock())
		link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getGroup() const
{
	QReadLocker scopedLocker(&_lock);

	return _group;
}

void OsmAnd::FavoriteLocation_P::setGroup(const QString& newGroup)
{
	QWriteLocker scopedLocker(&_lock);

	_group = newGroup;

	if (const auto link = _weakLink.lock())
		link->_p->notifyFavoriteLocationChanged(owner);
}

OsmAnd::ColorRGB OsmAnd::FavoriteLocation_P::getColor() const
{
	QReadLocker scopedLocker(&_lock);

	return _color;
}

void OsmAnd::FavoriteLocation_P::setColor(const ColorRGB newColor)
{
	QWriteLocker scopedLocker(&_lock);

	_color = newColor;

	if (const auto link = _weakLink.lock())
		link->_p->notifyFavoriteLocationChanged(owner);
}

void OsmAnd::FavoriteLocation_P::detach()
{
	_weakLink.reset();
}
