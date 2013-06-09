#ifndef OSMANDRASTERMAPLAYER_H
#define OSMANDRASTERMAPLAYER_H

#include <OsmAndCore.h>
#include <OsmAndMapDataLayer.h>
#include <Utilities.h>
#include <QMap>
#include <QSet>
#include <SkBitmap.h>
#include <OsmAndMapTileSource.h>

namespace OsmAnd {


class RasterMapDownloader;
class OSMAND_CORE_API OsmAndRasterMapLayer : public OsmAndMapDataLayer
{
private :
    std::shared_ptr<OsmAnd::OsmAndApplication> app;
    std::shared_ptr<MapTileSource> tileSource;
    SkBitmap* img;
    MapPoint* topLeft;
    MapPoint* bottomRight;
    SkBitmap* doubleBuffer;
    QMap<QString, std::shared_ptr<SkBitmap> > cache;
    QMap<QString, int> cacheValues;
    QSet<QString> requestedToDownload;


    std::shared_ptr<SkBitmap> loadImage(int x, int y, int z, QString& s);
    void renderRaster(OsmAnd::OsmAndMapView* view, const QString& appDir);
    void clearCachePartially();

public:

    OsmAndRasterMapLayer(std::shared_ptr<OsmAnd::OsmAndApplication> app) : app(app), img(nullptr), doubleBuffer(nullptr)
    {
    }
    virtual ~OsmAndRasterMapLayer() {}
    virtual void clearCache() ;
    virtual void updateViewport(OsmAnd::OsmAndMapView* view);

    std::shared_ptr<MapTileSource> getTileSource() { return tileSource; }
    void setTileSource(std::shared_ptr<MapTileSource> tileSource);

    SkBitmap* getBitmap(MapPoint* topLeft, MapPoint* bottomRight);
    friend class RasterMapDownloader;
};
}
#endif // OSMANDRASTERMAPLAYER_H
