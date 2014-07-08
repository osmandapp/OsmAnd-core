#ifndef _OSMAND_CORE_FAVORITE_LOCATION_H_
#define _OSMAND_CORE_FAVORITE_LOCATION_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "Link.h"
#include "CommonTypes.h"
#include "IFavoriteLocation.h"

namespace OsmAnd
{
    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P;

    class FavoriteLocation_P;
    class FavoriteLocation : public IFavoriteLocation
    {
        Q_DISABLE_COPY(FavoriteLocation);

    private:
        PrivateImplementation<FavoriteLocation_P> _p;
    protected:
        FavoriteLocation(
            const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink,
            const PointI position,
            const QString& title,
            const QString& group,
            const ColorRGB color);

        void attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink);
        void detach();
    public:
        FavoriteLocation(const PointI position);
        virtual ~FavoriteLocation();

        const PointI position;
        virtual PointI getPosition() const;

        virtual bool isHidden() const;
        virtual void setIsHidden(const bool isHidden);

        virtual QString getTitle() const;
        virtual void setTitle(const QString& newTitle);

        virtual QString getGroup() const;
        virtual void setGroup(const QString& newGroup);

        virtual ColorRGB getColor() const;
        virtual void setColor(const ColorRGB newColor);

    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocationsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATION_H_)
