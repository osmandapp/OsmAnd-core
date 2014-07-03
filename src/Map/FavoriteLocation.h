#ifndef _OSMAND_CORE_FAVORITE_LOCATION_H_
#define _OSMAND_CORE_FAVORITE_LOCATION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Link.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IFavoriteLocation.h>

namespace OsmAnd
{
    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P;

    class FavoriteLocation_P;
    class OSMAND_CORE_API FavoriteLocation : public IFavoriteLocation
    {
        Q_DISABLE_COPY(FavoriteLocation);

    private:
        PrivateImplementation<FavoriteLocation_P> _p;
    protected:
        FavoriteLocation(
			Link<FavoriteLocationsCollection*>& containerLink,
            const PointI position,
            const QString& title,
            const QString& group,
            const ColorRGB color);

		void detach();
    public:
        virtual ~FavoriteLocation();

        const PointI position;
        virtual PointI getPosition() const;

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
