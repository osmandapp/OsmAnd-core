#ifndef _OSMAND_CORE_POLYGONS_COLLECTION_P_H_
#define _OSMAND_CORE_POLYGONS_COLLECTION_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QList>
#include <QVector>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapKeyedSymbolsProvider.h"
#include "Polygon.h"
#include "PolygonBuilder.h"

namespace OsmAnd
{
    class PolygonBuilder;
    class PolygonBuilder_P;

    class PolygonsCollection;
    class PolygonsCollection_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(PolygonsCollection_P);

    private:
    protected:
        PolygonsCollection_P(PolygonsCollection* const owner);

        mutable QReadWriteLock _polygonsLock;
        QHash< IMapKeyedSymbolsProvider::Key, std::shared_ptr<Polygon> > _polygons;

        bool addPolygon(const std::shared_ptr<Polygon>& polygon);
    public:
        virtual ~PolygonsCollection_P();

        ImplementationInterface<PolygonsCollection> owner;

        QList< std::shared_ptr<Polygon> > getPolygons() const;
        bool removePolygon(const std::shared_ptr<Polygon>& polygon);
        void removeAllPolygons();

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData);

    friend class OsmAnd::PolygonsCollection;
    friend class OsmAnd::PolygonBuilder;
    friend class OsmAnd::PolygonBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_POLYGONS_COLLECTION_P_H_)
