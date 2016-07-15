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
        Q_DISABLE_COPY_AND_MOVE(FavoriteLocation_P)

    private:
    protected:
        FavoriteLocation_P(FavoriteLocation* const owner);

        Link<FavoriteLocationsCollection*>::WeakEnd _weakLink;

        mutable QReadWriteLock _lock;

        bool _isHidden;
        QString _title;
        QString _description;
        QString _group;
        ColorRGB _color;

        void attach(const std::shared_ptr< Link<FavoriteLocationsCollection*> >& containerLink);
        void detach();
    public:
        virtual ~FavoriteLocation_P();

        ImplementationInterface<FavoriteLocation> owner;

        bool isHidden() const;
        void setIsHidden(const bool isHidden);

        QString getTitle() const;
        void setTitle(const QString& newTitle);

        QString getDescription() const;
        void setDescription(const QString& newDescription);

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
