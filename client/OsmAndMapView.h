#ifndef MAPVIEW_H
#define MAPVIEW_H
#include <QRect>
#include <QRectF>
#include <QList>

#include "OsmAndCore.h"
#include "Utilities.h"
#include "OsmAndApplication.h"
#include <OsmAndMapDataLayer.h>

namespace OsmAnd {

const int CENTER_POSITION = 0;
const int BOTTOM_POSITION = 1;
const int LOWEST_ZOOM_TO_ROTATE = 3;
class OsmAndMapDataLayer;

class OSMAND_CORE_API OsmAndMapView
{
private :
    float zoom ;
    double longitude ;
    double latitude;
    int width;
    int height;
    float rotate ;
    float rotateSin;
    float rotateCos;
    int mapPosition;

    std::shared_ptr<OsmAnd::OsmAndApplication> app;
    QList<std::shared_ptr<OsmAnd::OsmAndMapDataLayer> > layers;



    void updateLayersViewport();
    // deprecated
    int getMapXForPoint(double longitude);
    int getMapYForPoint(double latitude);


public:
    float calcDiffTileY(float dx, float dy);
    float calcDiffTileX(float dx, float dy);
    float calcDiffPixelY(float dx, float dy);
    float calcDiffPixelX(float dx, float dy);

    OsmAndMapView(const std::shared_ptr<OsmAnd::OsmAndApplication>&);

    inline float getZoom() {return zoom;}

    void setZoom(float);

    inline double getLatitude() { return latitude; }

    inline double getLongitude() { return longitude; }

    inline float getXTile() {return (float) OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);}

    inline float getYTile() {return (float) OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);}

    void setBounds(int w, int h);

    int getWidth() { return width; }

    int getHeight() { return  height; }

    float getRotate();

    void setRotate(float r);

    bool isMapRotateEnabled();

    float getTileSize();

    int getSourceTileSize();

    int getCenterPointX();

    int getCenterPointY();

    QRectF getTileRect();

    void calculateTileRectangle(QRect& pixRect, float cx, float cy, float ctilex, float ctiley, QRectF& tileRect);

    float getRotatedMapLatForPoint(float x, float y);
    float getRotatedMapLonForPoint(float x, float y);


    PointI getPixelPointForLatLon(double latitude, double longitude);
    PointI getPixelPoint(int32_t x31, int32_t y31);

    bool isPoint31OnTheRotatedMap(int32_t x31, int32_t y31);
    bool isLatLonPointOnTheRotatedMap(double latitude, double longitude);


    void moveTo(float dx, float dy);

    void setLatLon(double latitiude, double longitude);

    void addLayer(std::shared_ptr<OsmAndMapDataLayer> layer);
    // not used now
    // inline float getEllipticYTile() {return (float) OsmAnd::Utilities::getTileEllipsoidNumberY(getZoom(), latitude);}
};

class OSMAND_CORE_API MapPoint {
public:
    const int32_t x31;
    const int32_t y31;

    PointI pixel(OsmAndMapView* v)
    {
        return v->getPixelPoint(x31, y31);
    }


};

}
#endif // MAPVIEW_H
