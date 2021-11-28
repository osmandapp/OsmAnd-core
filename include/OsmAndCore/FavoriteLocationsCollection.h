#ifndef _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_
#define _OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IFavoriteLocationsCollection.h>

namespace OsmAnd
{
    class IFavoriteLocation;
    class FavoriteLocation;
    class FavoriteLocation_P;

    class FavoriteLocationsCollection_P;
    class OSMAND_CORE_API FavoriteLocationsCollection : public IFavoriteLocationsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocationsCollection);

    private:
        PrivateImplementation<FavoriteLocationsCollection_P> _p;
    protected:
        FavoriteLocationsCollection(FavoriteLocationsCollection_P* const p);
    public:
        FavoriteLocationsCollection();
        virtual ~FavoriteLocationsCollection();

        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const PointI position31,
            const QString& elevation = {},
            const QString& time = {},
            const QString& creationTime = {},
            const QString& title = {},
            const QString& description = {},
            const QString& address = {},
            const QString& group = {},
            const QString& icon = {},
            const QString& background = {},
            const ColorRGB color = ColorRGB());
        virtual std::shared_ptr<IFavoriteLocation> createFavoriteLocation(
            const LatLon latLon,
            const QString& elevation = {},
            const QString& time = {},
            const QString& creationTime = {},
            const QString& title = {},
            const QString& description = {},
            const QString& address = {},
            const QString& group = {},
            const QString& icon = {},
            const QString& background = {},
            const ColorRGB color = ColorRGB());
        
        virtual std::shared_ptr<IFavoriteLocation> copyFavoriteLocation(const std::shared_ptr<const IFavoriteLocation>& other);
        virtual bool removeFavoriteLocation(const std::shared_ptr<IFavoriteLocation>& favoriteLocation);
        virtual bool removeFavoriteLocations(const QList< std::shared_ptr<IFavoriteLocation> >& favoriteLocations);
        virtual void clearFavoriteLocations();

        virtual unsigned int getFavoriteLocationsCount() const;
        virtual QList< std::shared_ptr<IFavoriteLocation> > getFavoriteLocations() const;

        virtual QSet<QString> getGroups() const;

        virtual void copyFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection);
        virtual void copyFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection);
        virtual void copyFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection);
        virtual void mergeFrom(const std::shared_ptr<const IFavoriteLocationsCollection>& otherCollection);
        virtual void mergeFrom(const QList< std::shared_ptr<IFavoriteLocation> >& otherCollection);
        virtual void mergeFrom(const QList< std::shared_ptr<const IFavoriteLocation> >& otherCollection);

    friend class OsmAnd::FavoriteLocation;
    friend class OsmAnd::FavoriteLocation_P;
    };
}

#endif // !defined(_OSMAND_CORE_FAVORITE_LOCATIONS_COLLECTION_H_)
