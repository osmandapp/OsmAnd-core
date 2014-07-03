#ifndef _OSMAND_CORE_FAVORITE_LOCATION_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATION_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QReadWriteLock>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Link.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IFavoriteLocation.h>

namespace OsmAnd
{
    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P;

    class FavoriteLocation;
    class FavoriteLocation_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY(FavoriteLocation_P);

    private:
    protected:
		FavoriteLocation_P(FavoriteLocation* const owner);

		Link<FavoriteLocationsCollection*>::WeakEndT _weakLink;

        mutable QReadWriteLock _lock;

        QString _title;
        QString _group;
        ColorRGB _color;

		void attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink);
		void detach();
    public:
        virtual ~FavoriteLocation_P();

        ImplementationInterface<FavoriteLocation> owner;

        QString getTitle() const;
        void setTitle(const QString& newTitle);

        QString getGroup() const;
        void setGroup(const QString& newGroup);

        ColorRGB getColor() const;
        void setColor(const ColorRGB newColor);

    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocationsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATION_P_H_)
