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
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocation)

    private:
        PrivateImplementation<FavoriteLocation_P> _p;
    protected:
        FavoriteLocation(
            const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink,
            const PointI position31,
            const QString& title,
            const QString& description,
            const QString& group,
            const ColorRGB color);

        FavoriteLocation(
            const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink,
            const LatLon latLon,
            const QString& title,
            const QString& description,
            const QString& group,
            const ColorRGB color);

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

        virtual QString getTitle() const;
        virtual void setTitle(const QString& newTitle);

        virtual QString getDescription() const;
        virtual void setDescription(const QString& newDescription);

        virtual QString getGroup() const;
        virtual void setGroup(const QString& newGroup);

        virtual ColorRGB getColor() const;
        virtual void setColor(const ColorRGB newColor);

    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocationsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATION_H_)
