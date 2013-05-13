#include "MapView.h"
#include <math.h>
namespace OsmAnd {

MapView::MapView(std::shared_ptr<OsmAnd::OsmAndApplication> app ) : zoom(3), longitude(0), latitude(0),
    rotate(0), rotateCos(1), rotateSin(0), app(app),
    mapPosition(CENTER_POSITION), width(0), height(0)
{

}

float MapView::getRotate() {
    return isMapRotateEnabled() ? rotate : 0;
}

void MapView::setZoom(float z) {
    this->zoom = z;
}

bool MapView::isMapRotateEnabled(){
    return zoom > LOWEST_ZOOM_TO_ROTATE;
}

void MapView::setBounds(int w, int h) {
    width = w;
    height = h;
}

void MapView::setLatLon(double latitiude, double longitude){
    // nanimatedDraggingThread.stopAnimating();
    this->latitude = latitiude;
    this->longitude = longitude;
}

float MapView::getTileSize() {
    float res = getSourceTileSize();
    if (zoom != (int) zoom) {
        res *= (float) pow(2, zoom - (int) zoom);
    }
    return res;
}

float MapView::calcDiffTileY(float dx, float dy) {
    if(isMapRotateEnabled()){
        return (-rotateSin * dx + rotateCos * dy) / getTileSize();
    } else {
        return dy / getTileSize();
    }

}

float MapView::calcDiffTileX(float dx, float dy) {
    if(isMapRotateEnabled()){
        return (rotateCos * dx + rotateSin * dy) / getTileSize();
    } else {
        return dx / getTileSize();
    }
}

float MapView::calcDiffPixelY(float dTileX, float dTileY) {
    if(isMapRotateEnabled()){
        return (rotateSin * dTileX + rotateCos * dTileY) * getTileSize();
    } else {
        return dTileY * getTileSize();
    }

}

float MapView::calcDiffPixelX(float dTileX, float dTileY) {
    if(isMapRotateEnabled()){
        return (rotateCos * dTileX - rotateSin * dTileY) * getTileSize();
    } else {
        return dTileX * getTileSize();
    }
}


int MapView::getSourceTileSize() {
    int r = 256 * app->getSettings()->MAP_SCALE.get().toFloat();
    return r;
}

QRectF MapView::getTileRect() {
    int z = (int) zoom;
    float tileX = (float) OsmAnd::Utilities::getTileNumberX(z, getLongitude());
    float tileY = (float) OsmAnd::Utilities::getTileNumberY(z, getLatitude());
    float w = getCenterPointX();
    float h = getCenterPointY();
    QRect pixRect(0, 0, width, height);
    QRectF tilesRect;
    calculateTileRectangle(pixRect, w, h, tileX, tileY, tilesRect);
    return tilesRect;
}

int MapView::getCenterPointX() {
    return getWidth() / 2;
}

int MapView::getCenterPointY() {
    if (mapPosition == BOTTOM_POSITION) {
        return 3 * getHeight() / 4;
    }
    return getHeight() / 2;
}

void MapView::calculateTileRectangle(QRect& pixRect, float cx, float cy, float ctilex, float ctiley, QRectF& tileRect) {

    float x1 = calcDiffTileX(pixRect.left() - cx, pixRect.top() - cy);
    float x2 = calcDiffTileX(pixRect.left() - cx, pixRect.bottom() - cy);
    float x3 = calcDiffTileX(pixRect.right() - cx, pixRect.top() - cy);
    float x4 = calcDiffTileX(pixRect.right() - cx, pixRect.bottom() - cy);
    float y1 = calcDiffTileY(pixRect.left() - cx, pixRect.top() - cy);
    float y2 = calcDiffTileY(pixRect.left() - cx, pixRect.bottom() - cy);
    float y3 = calcDiffTileY(pixRect.right() - cx, pixRect.top() - cy);
    float y4 = calcDiffTileY(pixRect.right() - cx, pixRect.bottom() - cy);
    float l = qMin(qMin(x1, x2), qMin(x3, x4)) + ctilex;
    float r = qMax(qMax(x1, x2), qMax(x3, x4)) + ctilex;
    float t = qMin(qMin(y1, y2), qMin(y3, y4)) + ctiley;
    float b = qMax(qMax(y1, y2), qMax(y3, y4)) + ctiley;
    tileRect.setCoords(l, t, r, b);
}


int MapView::getMapXForPoint(double longitude) {
    double tileX = OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);
    return (int) ((tileX - getXTile()) * getTileSize() + getCenterPointX());
}

int MapView::getMapYForPoint(double latitude) {
    double tileY = OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);
    return (int) ((tileY - getYTile()) * getTileSize() + getCenterPointY());
}

int MapView::getRotatedMapXForPoint(double latitude, double longitude) {
    int cx = getCenterPointX();
    double xTile = OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);
    double yTile = OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);
    return (int) (calcDiffPixelX((float) (xTile - getXTile()), (float) (yTile - getYTile())) + cx);
}

int MapView::getRotatedMapYForPoint(double latitude, double longitude) {
    int cy = getCenterPointY();
    double xTile = OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);
    double yTile = OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);
    return (int) (calcDiffPixelY((float) (xTile - getXTile()), (float) (yTile - getYTile())) + cy);
}

bool MapView::isPointOnTheRotatedMap(double latitude, double longitude) {
    int cx = getCenterPointX();
    int cy = getCenterPointY();
    double xTile = OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);
    double yTile = OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);
    int newX = (int) (calcDiffPixelX((float) (xTile - getXTile()), (float) (yTile - getYTile())) + cx);
    int newY = (int) (calcDiffPixelY((float) (xTile - getXTile()), (float) (yTile - getYTile())) + cy);
    if (newX >= 0 && newX <= getWidth() && newY >= 0 && newY <= getHeight()) {
        return true;
    }
    return false;
}

void MapView::moveTo(float dx, float dy) {
    float fy = calcDiffTileY(dx, dy);
    float fx = calcDiffTileX(dx, dy);

    this->latitude = OsmAnd::Utilities::getLatitudeFromTile(getZoom(), getYTile() + fy);
    this->longitude = OsmAnd::Utilities::getLongitudeFromTile(getZoom(), getXTile() + fx);
}

}
