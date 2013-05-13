#ifndef MAPVIEW_H
#define MAPVIEW_H
#include <QRect>
#include <QRectF>

#include "OsmAndCore.h"
#include "Utilities.h"
#include "OsmAndApplication.h"

namespace OsmAnd {

const int CENTER_POSITION = 0;
const int BOTTOM_POSITION = 1;
const int LOWEST_ZOOM_TO_ROTATE = 10;

class OSMAND_CORE_API MapView
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

    float calcDiffTileY(float dx, float dy);
    float calcDiffTileX(float dx, float dy);
    float calcDiffPixelY(float dx, float dy);
    float calcDiffPixelX(float dx, float dy);
    int getMapXForPoint(double longitude);
    int getMapYForPoint(double latitude);


public:
    MapView(std::shared_ptr<OsmAnd::OsmAndApplication>);

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

    bool isMapRotateEnabled();

    float getTileSize();

    int getSourceTileSize();

    int getCenterPointX();

    int getCenterPointY();

    QRectF getTileRect();

    void calculateTileRectangle(QRect& pixRect, float cx, float cy, float ctilex, float ctiley, QRectF& tileRect);

    int getRotatedMapXForPoint(double latitude, double longitude);

    int getRotatedMapYForPoint(double latitude, double longitude);

    bool isPointOnTheRotatedMap(double latitude, double longitude);

    void moveTo(float dx, float dy);

    void setLatLon(double latitiude, double longitude);
    // not used now
    // inline float getEllipticYTile() {return (float) OsmAnd::Utilities::getTileEllipsoidNumberY(getZoom(), latitude);}
};

}
#endif // MAPVIEW_H
