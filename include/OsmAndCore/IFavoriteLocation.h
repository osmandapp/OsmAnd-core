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

namespace OsmAnd
{
    class OSMAND_CORE_API IFavoriteLocation
    {
        Q_DISABLE_COPY_AND_MOVE(IFavoriteLocation)
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

        virtual QString getTitle() const = 0;
        virtual void setTitle(const QString& newTitle) = 0;

        virtual QString getDescription() const = 0;
        virtual void setDescription(const QString& newDescription) = 0;

        virtual QString getGroup() const = 0;
        virtual void setGroup(const QString& newGroup) = 0;

        virtual ColorRGB getColor() const = 0;
        virtual void setColor(const ColorRGB newColor) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_FAVORITE_LOCATION_H_)
