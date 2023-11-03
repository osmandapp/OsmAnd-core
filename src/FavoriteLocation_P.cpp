#include "FavoriteLocation_P.h"
#include "FavoriteLocation.h"

#include "FavoriteLocationsCollection.h"
#include "FavoriteLocationsCollection_P.h"

OsmAnd::FavoriteLocation_P::FavoriteLocation_P(FavoriteLocation* const owner_)
    : _isHidden(false)
    , _calendarEvent(false)
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

double OsmAnd::FavoriteLocation_P::getElevation() const
{
    QReadLocker scopedLocker(&_lock);

    return _elevation;
}

void OsmAnd::FavoriteLocation_P::setElevation(const double newElevation)
{
    QWriteLocker scopedLocker(&_lock);

    _elevation = newElevation;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getTime() const
{
    QReadLocker scopedLocker(&_lock);

    return _time;
}

void OsmAnd::FavoriteLocation_P::setTime(const QString& newTime)
{
    QWriteLocker scopedLocker(&_lock);

    _time = newTime;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getPickupTime() const
{
    QReadLocker scopedLocker(&_lock);

    return _pickupTime;
}

void OsmAnd::FavoriteLocation_P::setPickupTime(const QString& newTime)
{
    QWriteLocker scopedLocker(&_lock);

    _pickupTime = newTime;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

bool OsmAnd::FavoriteLocation_P::getCalendarEvent() const
{
    QReadLocker scopedLocker(&_lock);

    return _calendarEvent;
}

void OsmAnd::FavoriteLocation_P::setCalendarEvent(const bool calendarEvent)
{
    QWriteLocker scopedLocker(&_lock);

    _calendarEvent = calendarEvent;

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

QString OsmAnd::FavoriteLocation_P::getDescription() const
{
    QReadLocker scopedLocker(&_lock);

    return _description;
}

void OsmAnd::FavoriteLocation_P::setDescription(const QString& newDescription)
{
    QWriteLocker scopedLocker(&_lock);

    _description = newDescription;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getAddress() const
{
    QReadLocker scopedLocker(&_lock);

    return _address;
}

void OsmAnd::FavoriteLocation_P::setAddress(const QString& newAddress)
{
    QWriteLocker scopedLocker(&_lock);

    _address = newAddress;

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

OsmAnd::ColorARGB OsmAnd::FavoriteLocation_P::getColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _color;
}

void OsmAnd::FavoriteLocation_P::setColor(const ColorARGB newColor)
{
    QWriteLocker scopedLocker(&_lock);

    _color = newColor;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getIcon() const
{
    QReadLocker scopedLocker(&_lock);

    return _icon.isEmpty() ? QStringLiteral("special_star") : _icon;
}

void OsmAnd::FavoriteLocation_P::setIcon(const QString& newIcon)
{
    QWriteLocker scopedLocker(&_lock);

    _icon = newIcon;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getBackground() const
{
    QReadLocker scopedLocker(&_lock);

    return _background.isEmpty() ? QStringLiteral("circle") : _background;
}

void OsmAnd::FavoriteLocation_P::setBackground(const QString& newBackground)
{
    QWriteLocker scopedLocker(&_lock);

    _background = newBackground;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QHash<QString, QString> OsmAnd::FavoriteLocation_P::getExtensions() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _extensions;
}

void OsmAnd::FavoriteLocation_P::setExtensions(const QHash<QString, QString>& extensions)
{
    QWriteLocker scopedLocker(&_lock);
    
    _extensions = extensions;
}

QString OsmAnd::FavoriteLocation_P::getExtension(const QString& tag)
{
    QReadLocker scopedLocker(&_lock);
    
    return _extensions[tag];
}

void OsmAnd::FavoriteLocation_P::setExtension(const QString& tag, const QString& value)
{
    QWriteLocker scopedLocker(&_lock);
    
    _extensions[tag] = value;
}

QString OsmAnd::FavoriteLocation_P::getComment() const
{
    QReadLocker scopedLocker(&_lock);

    return _comment;
}

void OsmAnd::FavoriteLocation_P::setComment(const QString& comment)
{
    QWriteLocker scopedLocker(&_lock);

    _comment = comment;

    if (const auto link = _weakLink.lock())
        link->_p->notifyFavoriteLocationChanged(owner);
}

QString OsmAnd::FavoriteLocation_P::getAmenityOriginName() const
{
    QReadLocker scopedLocker(&_lock);

    return _amenityOriginName;
}

void OsmAnd::FavoriteLocation_P::setAmenityOriginName(const QString &originName)
{
    QWriteLocker scopedLocker(&_lock);

    _amenityOriginName = originName;

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
