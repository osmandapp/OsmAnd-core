#ifndef _OSMAND_CORE_GEO_INFO_PRESENTER_P_H_
#define _OSMAND_CORE_GEO_INFO_PRESENTER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapObjectsProvider.h"
#include "GeoInfoPresenter.h"

namespace OsmAnd
{
    class GeoInfoDocument;
    class MapPresentationEnvironment;

    class GeoInfoPresenter_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(GeoInfoPresenter_P);

    public:
        typedef GeoInfoPresenter::MapObject MapObject;
        typedef GeoInfoPresenter::WaypointMapObject WaypointMapObject;
        typedef GeoInfoPresenter::TrackpointMapObject TrackpointMapObject;
        typedef GeoInfoPresenter::TracklineMapObject TracklineMapObject;
        typedef GeoInfoPresenter::RoutepointMapObject RoutepointMapObject;
        typedef GeoInfoPresenter::RoutelineMapObject RoutelineMapObject;

    private:
    protected:
        GeoInfoPresenter_P(GeoInfoPresenter* const owner);

        static QList< std::shared_ptr<const MapObject> > generateMapObjects(
            const QList< std::shared_ptr<const GeoInfoDocument> >& geoInfoDocuments);
    public:
        virtual ~GeoInfoPresenter_P();

        ImplementationInterface<GeoInfoPresenter> owner;

        std::shared_ptr<IMapObjectsProvider> createMapObjectsProvider() const;

    friend class OsmAnd::GeoInfoPresenter;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_INFO_PRESENTER_P_H_)
