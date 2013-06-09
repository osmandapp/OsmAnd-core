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
    int getMapXForPoint(double longitude) const;
    int getMapYForPoint(double latitude) const;


public:
    float calcDiffTileY(float dx, float dy) const;
    float calcDiffTileX(float dx, float dy) const;
    float calcDiffPixelY(float dx, float dy) const;
    float calcDiffPixelX(float dx, float dy) const;

    OsmAndMapView(const std::shared_ptr<OsmAnd::OsmAndApplication>&);

    inline float getZoom() const {return zoom;}

    void setZoom(float);

    inline double getLatitude() const { return latitude; }

    inline double getLongitude() const { return longitude; }

    inline float getXTile() const {return (float) OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);}

    inline float getYTile() const {return (float) OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);}

    void setBounds(int w, int h);

    int getWidth() const { return width; }

    int getHeight() const { return  height; }

    float getRotate() const ;

    void setRotate(float r);

    bool isMapRotateEnabled() const ;

    float getTileSize() const ;

    int getSourceTileSize() const ;

    int getCenterPointX() const ;

    int getCenterPointY() const ;

    QRectF getTileRect() const;

    void calculateTileRectangle(QRect& pixRect, float cx, float cy, float ctilex, float ctiley, QRectF& tileRect) const ;

    float getRotatedMapLatForPoint(float x, float y) const ;
    float getRotatedMapLonForPoint(float x, float y) const ;


    PointI getPixelPointForLatLon(double latitude, double longitude) const ;
    PointI getPixelPoint(int32_t x31, int32_t y31) const ;

    bool isPoint31OnTheRotatedMap(int32_t x31, int32_t y31) const;
    bool isLatLonPointOnTheRotatedMap(double latitude, double longitude) const;


    void moveTo(float dx, float dy);

    void setLatLon(double latitiude, double longitude);

    void addLayer(std::shared_ptr<OsmAndMapDataLayer> layer);
    // not used now
    // inline float getEllipticYTile() {return (float) OsmAnd::Utilities::getTileEllipsoidNumberY(getZoom(), latitude);}
};

class OSMAND_CORE_API MapPoint {
public:
    int32_t x31;
    int32_t y31;
    OsmAndMapView* v;
    MapPoint(OsmAndMapView* v, int32_t x31,int32_t y31) : x31(x31), y31(y31), v(v) {}
    MapPoint() : x31(0), y31(0), v(nullptr) {}


    PointI pixel()
    {
        if(v == nullptr) {
            return PointI(0, 0);
        }
        return v->getPixelPoint(x31, y31);
    }


};

}
#endif // MAPVIEW_H
