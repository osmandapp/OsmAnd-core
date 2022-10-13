#ifndef _OSMAND_CORE_FAVORITE_LOCATION_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "Link.h"
#include "CommonTypes.h"
#include "Color.h"
#include "IFavoriteLocation.h"

namespace OsmAnd
{
    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P;

    class FavoriteLocation;
    class FavoriteLocation_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocation_P);

    private:
    protected:
        FavoriteLocation_P(FavoriteLocation* const owner);

        Link<FavoriteLocationsCollection*>::WeakEnd _weakLink;

        mutable QReadWriteLock _lock;

        bool _isHidden;
        QString _elevation;
        QString _time;
        QString _pickupTime;
        QString _title;
        QString _description;
        QString _address;
        QString _group;
        QString _icon;
        QString _background;
        ColorRGB _color;
        QString _comment;
        QString _amenityOriginName;
        QHash<QString, QString> _extensions;
        bool _calendarEvent;

        void attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink);
        void detach();
    public:
        virtual ~FavoriteLocation_P();

        ImplementationInterface<FavoriteLocation> owner;

        bool isHidden() const;
        void setIsHidden(const bool isHidden);
        
        QString getElevation() const;
        void setElevation(const QString& newElevation);
        
        QString getTime() const;
        void setTime(const QString& newTitle);
        
        QString getPickupTime() const;
        void setPickupTime(const QString& newTitle);

        QString getTitle() const;
        void setTitle(const QString& newTime);
        
        bool getCalendarEvent() const;
        void setCalendarEvent(const bool calendarEvent);

        QString getDescription() const;
        void setDescription(const QString& newDescription);
        
        QString getAddress() const;
        void setAddress(const QString& newAddress);

        QString getGroup() const;
        void setGroup(const QString& newGroup);

        QString getIcon() const;
        void setIcon(const QString& newIcon);
        
        QString getBackground() const;
        void setBackground(const QString& newBackground);
        
        ColorRGB getColor() const;
        void setColor(const ColorRGB newColor);
        
        QHash<QString, QString> getExtensions() const;
        void setExtensions(const QHash<QString, QString>& extensions);
        
        QString getExtension(const QString& tag);
        void setExtension(const QString& tag, const QString& value);
        
        virtual QString getComment() const;
        virtual void setComment(const QString& comment);
        
        virtual QString getAmenityOriginName() const;
        virtual void setAmenityOriginName(const QString& originName);

    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocationsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATION_P_H_)
