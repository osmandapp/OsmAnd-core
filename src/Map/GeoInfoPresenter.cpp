#include "GeoInfoPresenter.h"
#include "GeoInfoPresenter_P.h"

OsmAnd::GeoInfoPresenter::GeoInfoPresenter(
    const QList< std::shared_ptr<const GeoInfoDocument> >& documents_,
    const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment_)
    :// _p(/*new GeoInfoPresenter_P(this)*/ nullptr)
    /*,*/ documents(documents_)
    , mapPresentationEnvironment(mapPresentationEnvironment_)
{
}

OsmAnd::GeoInfoPresenter::~GeoInfoPresenter()
{
}

std::shared_ptr<OsmAnd::IMapLayerProvider> OsmAnd::GeoInfoPresenter::createMapLayerProvider() const
{
    return nullptr;
}
