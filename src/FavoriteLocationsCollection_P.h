#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>
#include <QList>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "Link.h"
#include "CommonTypes.h"
#include "PointsAndAreas.h"
#include "Color.h"
#include "LatLon.h"

namespace OsmAnd
{
    class IFavoriteLocation;
    class FavoriteLocation;
    class FavoriteLocation_P;

    class FavoriteLocationsCollection;
    class FavoriteLocationsCollection_P
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsCollection_P);

    private:
    protected:
        FavoriteLocationsCollection_P(FavoriteLocationsCollection* const owner);

        typedef OsmAnd::Link<FavoriteLocationsCollection*> Link;
        std::shared_ptr<Link> _containerLink;

        mutable QReadWriteLock _collectionLock;
        QHash< FavoriteLocation*, std::shared_ptr<FavoriteLocation> > _collection;

        void notifyCollectionChanged();
        void notifyFavoriteLocationChanged(FavoriteLocation* const pFavoriteLocation);

        void doClearFavoriteLocations();
        void appendFrom(const QList< std::shared_ptr<FavoriteLocation> >& collection);
    public:
        virtual ~FavoriteLocationsCollection_P();

        ImplementationInterface<FavoriteLocationsCollection> owner;

        std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position,
            const QString& elevation,
            const QString& time,
            const QString& creationTime,
            const QString& title,
            const QString& description,
            const QString& address,
            const QString& group,
            const QString& icon,
            const QString& background,
            const ColorARGB color,
            const QHash<QString, QString>& extensions,
            const bool calendarEvent,
            const QString& amenityOriginName);
        std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
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
            const ColorARGB color,
            const QHash<QString, QString>& extensions,
            const bool calendarEvent,
            const QString& amenityOriginName);
        void addFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation);
        bool removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation);
        bool removeFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations);
        void clearFavoriteLocations();

        unsigned int getFavoriteLocationsCount() const;
        QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const;

        QSet<QString> getGroups() const;
        QHash<QString, QList<std::shared_ptr<OsmAnd::IFavoriteLocation>>> getGroupsLocations() const;

        void copyFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection);
        void copyFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection);
        void mergeFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection);
        void mergeFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection);

    friend class OsmAnd::FavoriteLocationsCollection;
    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocation_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_P_H_)
