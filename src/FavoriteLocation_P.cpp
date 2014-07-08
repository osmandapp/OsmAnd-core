#include "FavoriteLocation_P.h"
#include "FavoriteLocation.h"

#include "FavoriteLocationsCollection.h"
#include "FavoriteLocationsCollection_P.h"

OsmAnd::FavoriteLocation_P::FavoriteLocation_P(FavoriteLocation* const owner_)
    : _isHidden(false)
	, owner(owner_)
{
}

OsmAnd::FavoriteLocation_P::~FavoriteLocation_P()
{
}

bool OsmAnd::FavoriteLocation_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);

    return _isHidden;
}

void OsmAnd::FavoriteLocation_P::setIsHidden(const bool isHidden)
{
    QWriteLocker scopedLocker(&_lock);

    _isHidden = isHidden;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
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

void OsmAnd::FavoriteLocation_P::attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink)
{
	assert(!_weakLink);
	_weakLink = containerLink->getWeak();
}

void OsmAnd::FavoriteLocation_P::detach()
{
	_weakLink.reset();
}
