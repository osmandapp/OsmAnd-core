#ifndef _OSMAND_CORE_FAVORITE_LOCATION_H_
#define _OSMAND_CORE_FAVORITE_LOCATION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>

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

    class FavoriteLocation_P;
    class FavoriteLocation : public IFavoriteLocation
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocation);

    private:
        PrivateImplementation<FavoriteLocation_P> _p;
    protected:
        FavoriteLocation(
            const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink,
            const PointI position31,
            const QString& elevation,
            const QString& time,
            const QString& creationTime,
            const QString& title,
            const QString& description,
            const QString& address,
            const QString& group,
            const QString& icon,
            const QString& background,
            const ColorRGB color,
            const QHash<QString, QString>& extensions,
            const bool calendarEvent);

        FavoriteLocation(
            const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink,
            const LatLon latLon,
            const QString& elevation,
            const QString& time,
            const QString& creationTime,
            const QString& title,
            const QString& description,
            const QString& address,
            const QString& group,
            const QString& icon,
            const QString& background,
            const ColorRGB color,
            const QHash<QString, QString>& extensions,
            const bool calendarEvent);

        void attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink);
        void detach();
    public:
        FavoriteLocation(const PointI position31);
        FavoriteLocation(const LatLon latLon);
        virtual ~FavoriteLocation();

        LocationSource locationSource;
        virtual LocationSource getLocationSource() const;

        const PointI position31;
        virtual PointI getPosition31() const;

        const LatLon latLon;
        virtual LatLon getLatLon() const;

        virtual bool isHidden() const;
        virtual void setIsHidden(const bool isHidden);
        
        virtual QString getElevation() const;
        virtual void setElevation(const QString& newElevation);
        
        virtual QString getTime() const;
        virtual void setTime(const QString& newTime);
        
        virtual QString getPickupTime() const;
        virtual void setPickupTime(const QString& newTime);
        
        virtual bool getCalendarEvent() const;
        virtual void setCalendarEvent(const bool calendarEvent);

        virtual QString getTitle() const;
        virtual void setTitle(const QString& newTitle);

        virtual QString getDescription() const;
        virtual void setDescription(const QString& newDescription);
        
        virtual QString getAddress() const;
        virtual void setAddress(const QString& newAddress);

        virtual QString getGroup() const;
        virtual void setGroup(const QString& newGroup);

        virtual QString getIcon() const;
        virtual void setIcon(const QString& newIcon);
        
        virtual QString getBackground() const;
        virtual void setBackground(const QString& newBackground);
        
        virtual ColorRGB getColor() const;
        virtual void setColor(const ColorRGB newColor);
        
        virtual QHash<QString, QString> getExtensions() const;
        virtual void setExtensions(const QHash<QString, QString>& extensions);
        
        virtual QString getExtension(const QString& tag);
        virtual void setExtension(const QString& tag, const QString& value);
        
        virtual QString getComment() const;
        virtual void setComment(const QString& comment);
        
        virtual QString getAmenityOriginName() const;
        virtual void setAmenityOriginName(const QString& originName);

    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocationsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATION_H_)
