#ifndef OSMANDMAPDATALAYER_H
#define OSMANDMAPDATALAYER_H
#include <OsmAndCore.h>
#include <OsmAndMapView.h>

namespace OsmAnd {

class OsmAndMapView;

class OSMAND_CORE_API OsmAndMapDataLayer
{
    OsmAndMapView* parent;
public:
    OsmAndMapDataLayer();

    virtual void attach(OsmAndMapView* parent) { this->parent = parent;}

    virtual void updateViewport(OsmAnd::OsmAndMapView* view) = 0;

    virtual void clearCache() {}

    virtual ~OsmAndMapDataLayer() {}
};
}
#endif // OSMANDMAPDATALAYER_H
