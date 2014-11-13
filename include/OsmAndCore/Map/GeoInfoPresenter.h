#ifndef _OSMAND_CORE_GEO_INFO_PRESENTER_H_
#define _OSMAND_CORE_GEO_INFO_PRESENTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/GeoInfoDocument.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/IMapLayerProvider.h>

namespace OsmAnd
{
    class GeoInfoPresenter_P;
    class OSMAND_CORE_API GeoInfoPresenter
    {
        Q_DISABLE_COPY_AND_MOVE(GeoInfoPresenter);

    private:
        //PrivateImplementation<GeoInfoPresenter_P> _p;
    protected:
    public:
        GeoInfoPresenter(
            const QList< std::shared_ptr<const GeoInfoDocument> >& documents,
            const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment);
        virtual ~GeoInfoPresenter();

        const QList< std::shared_ptr<const GeoInfoDocument> > documents;
        const std::shared_ptr<const MapPresentationEnvironment> mapPresentationEnvironment;

        std::shared_ptr<IMapLayerProvider> createMapLayerProvider() const;
        //TODO: markers std::shared_ptr<IMap> createMapSymbolsProvider() const;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_INFO_PRESENTER_H_)
