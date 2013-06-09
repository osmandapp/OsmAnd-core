#ifndef OSMANDRASTERMAPLAYER_H
#define OSMANDRASTERMAPLAYER_H

#include <OsmAndCore.h>
#include <OsmAndMapDataLayer.h>
#include <Utilities.h>
#include <QMap>
#include <SkBitmap.h>

namespace OsmAnd {


class OSMAND_CORE_API OsmAndRasterMapLayer : public OsmAnd::OsmAndMapDataLayer
{
private :
    std::shared_ptr<OsmAnd::OsmAndApplication> app;
    SkBitmap* img;
    MapPoint* topLeft;
    MapPoint* bottomRight;
    SkBitmap* doubleBuffer;
    QMap<QString, SkBitmap* > cache;

    const SkBitmap* loadImage(QString& s);
    void renderRaster(OsmAnd::OsmAndMapView* view, const QString& tileSource, const QString& appDir);

public:

    OsmAndRasterMapLayer(std::shared_ptr<OsmAnd::OsmAndApplication> app) : app(app), img(nullptr), doubleBuffer(nullptr)
    {
    }
    virtual ~OsmAndRasterMapLayer() {}
    virtual void updateViewport(OsmAnd::OsmAndMapView* view);

    SkBitmap* getBitmap(MapPoint* topLeft, MapPoint* bottomRight);
};
}
#endif // OSMANDRASTERMAPLAYER_H
