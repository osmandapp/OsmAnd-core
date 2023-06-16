#ifndef _OSMAND_CORE_I_FAVORITE_LOCATION_H_
#define _OSMAND_CORE_I_FAVORITE_LOCATION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>

static const int kFavoriteLatLonPrecision = 7;

namespace OsmAnd
{
    class OSMAND_CORE_API IFavoriteLocation
    {
        Q_DISABLE_COPY_AND_MOVE(IFavoriteLocation);
    public:
        enum class LocationSource
        {
            Point31,
            LatLon
        };

    private:
    protected:
        IFavoriteLocation();
    public:
        virtual ~IFavoriteLocation();

        virtual LocationSource getLocationSource() const = 0;
        virtual PointI getPosition31() const = 0;
        virtual LatLon getLatLon() const = 0;

        virtual bool isHidden() const = 0;
        virtual void setIsHidden(const bool isHidden) = 0;
        
        virtual QString getElevation() const = 0;
        virtual void setElevation(const QString& newElevation) = 0;
        
        virtual QString getTime() const = 0;
        virtual void setTime(const QString& newTime) = 0;
        
        virtual QString getPickupTime() const = 0;
        virtual void setPickupTime(const QString& newTime) = 0;
        
        virtual bool getCalendarEvent() const = 0;
        virtual void setCalendarEvent(const bool calendarEvent) = 0;

        virtual QString getTitle() const = 0;
        virtual void setTitle(const QString& newTitle) = 0;

        virtual QString getDescription() const = 0;
        virtual void setDescription(const QString& newDescription) = 0;
        
        virtual QString getAddress() const = 0;
        virtual void setAddress(const QString& newAddress) = 0;

        virtual QString getGroup() const = 0;
        virtual void setGroup(const QString& newGroup) = 0;

        virtual QString getIcon() const = 0;
        virtual void setIcon(const QString& newIcon) = 0;
        
        virtual QString getBackground() const = 0;
        virtual void setBackground(const QString& newBackground) = 0;
        
        virtual ColorARGB getColor() const = 0;
        virtual void setColor(const ColorARGB newColor) = 0;
        
        virtual QString getComment() const = 0;
        virtual void setComment(const QString& comment) = 0;
        
        virtual QString getAmenityOriginName() const = 0;
        virtual void setAmenityOriginName(const QString& originName) = 0;
        
        virtual QHash<QString, QString> getExtensions() const = 0;
        virtual void setExtensions(const QHash<QString, QString>& extensions) = 0;
        
        virtual QString getExtension(const QString& tag) = 0;
        virtual void setExtension(const QString& tag, const QString& value) = 0;

        virtual uint32_t hash() const;
        virtual bool isEqual(IFavoriteLocation* favoriteLocation) const;

        virtual LatLon getLatLonPrecised() const;
    };
}

#endif // !defined(_OSMAND_CORE_I_FAVORITE_LOCATION_H_)
