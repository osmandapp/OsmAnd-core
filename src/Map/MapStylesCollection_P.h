#ifndef _OSMAND_CORE_MAP_STYLES_COLLECTION_P_H_
#define _OSMAND_CORE_MAP_STYLES_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QHash>
#include <QList>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class MapStyle;

    class MapStylesCollection;
    class MapStylesCollection_P
    {
    private:
    protected:
        MapStylesCollection_P(MapStylesCollection* owner);

        QList< std::shared_ptr<MapStyle> > _order;
        QHash< QString, std::shared_ptr<MapStyle> > _styles;
        mutable QReadWriteLock _stylesLock;

        bool registerEmbeddedStyle(const QString& resourceName);
    public:
        virtual ~MapStylesCollection_P();

        ImplementationInterface<MapStylesCollection> owner;

        bool registerStyle(const QString& filePath);

        QList< std::shared_ptr<const MapStyle> > getCollection() const;
        std::shared_ptr<const MapStyle> getAsIsStyle(const QString& name) const;
        bool obtainBakedStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const;

    friend class OsmAnd::MapStylesCollection;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_COLLECTION_P_H_)
