#include "OsmAndMapView.h"
#include <qmath.h>
namespace OsmAnd {

OsmAndMapView::OsmAndMapView(const std::shared_ptr<OsmAnd::OsmAndApplication>& app ) : zoom(3), longitude(0), latitude(0),
    rotate(0), rotateCos(1), rotateSin(0), app(app),
    mapPosition(CENTER_POSITION), width(0), height(0)
{

}

float OsmAndMapView::getRotate() const{
    return isMapRotateEnabled() ? rotate : 0;
}

void OsmAndMapView::setRotate(float rotate) {
    if (isMapRotateEnabled()) {
        this->rotate = rotate;
        float rad = (rotate / 180 )* M_PI;
        rotateCos = cos(rad);
        rotateSin = sin(rad);
        updateLayersViewport();
        //			float diff = MapUtils.unifyRotationDiff(rotate, getRotate());
        //			if (Math.abs(diff) > 5) { // check smallest rotation
        //				animatedDraggingThread.startRotate(rotate);
        //			}
    }
}

void OsmAndMapView::setZoom(float z) {
    this->zoom = z;
    updateLayersViewport();
}

bool OsmAndMapView::isMapRotateEnabled() const{
    return zoom > LOWEST_ZOOM_TO_ROTATE;
}

void OsmAndMapView::setBounds(int w, int h) {
    width = w;
    height = h;
    updateLayersViewport();
}

void OsmAndMapView::setLatLon(double latitiude, double longitude){
    // nanimatedDraggingThread.stopAnimating();
    this->latitude = latitiude;
    this->longitude = longitude;
    updateLayersViewport();
}

float OsmAndMapView::getTileSize() const {
    float res = getSourceTileSize();
    if (zoom != (int) zoom) {
        res *= (float) pow(2, zoom - (int) zoom);
    }
    return res;
}

float OsmAndMapView::calcDiffTileY(float dx, float dy) const{
    if(isMapRotateEnabled()){
        return (-rotateSin * dx + rotateCos * dy) / getTileSize();
    } else {
        return dy / getTileSize();
    }

}

float OsmAndMapView::calcDiffTileX(float dx, float dy) const{
    if(isMapRotateEnabled()){
        return (rotateCos * dx + rotateSin * dy) / getTileSize();
    } else {
        return dx / getTileSize();
    }
}

float OsmAndMapView::calcDiffPixelY(float dTileX, float dTileY) const{
    if(isMapRotateEnabled()){
        return (rotateSin * dTileX + rotateCos * dTileY) * getTileSize();
    } else {
        return dTileY * getTileSize();
    }

}

float OsmAndMapView::calcDiffPixelX(float dTileX, float dTileY) const{
    if(isMapRotateEnabled()){
        return (rotateCos * dTileX - rotateSin * dTileY) * getTileSize();
    } else {
        return dTileX * getTileSize();
    }
}


int OsmAndMapView::getSourceTileSize() const{
    int r = 256 * app->getSettings()->MAP_SCALE.get().toFloat();
    return r;
}

QRectF OsmAndMapView::getTileRect() const {
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

void OsmAndMapView::updateLayersViewport() {
    for(std::shared_ptr<OsmAndMapDataLayer> l : layers) {
        l->updateViewport(this);
    }

}

int OsmAndMapView::getCenterPointX() const{
    return getWidth() / 2;
}

int OsmAndMapView::getCenterPointY() const{
    if (mapPosition == BOTTOM_POSITION) {
        return 3 * getHeight() / 4;
    }
    return getHeight() / 2;
}

void OsmAndMapView::calculateTileRectangle(QRect& pixRect, float cx, float cy, float ctilex, float ctiley, QRectF& tileRect) const{

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


int OsmAndMapView::getMapXForPoint(double longitude) const {
    double tileX = OsmAnd::Utilities::getTileNumberX(getZoom(), longitude);
    return (int) ((tileX - getXTile()) * getTileSize() + getCenterPointX());
}

int OsmAndMapView::getMapYForPoint(double latitude) const {
    double tileY = OsmAnd::Utilities::getTileNumberY(getZoom(), latitude);
    return (int) ((tileY - getYTile()) * getTileSize() + getCenterPointY());
}

float OsmAndMapView::getRotatedMapLatForPoint(float x, float y) const{
    float dx = x - getCenterPointX();
    float dy = y - getCenterPointY();
    float fy = calcDiffTileY(dx, dy);
    return OsmAnd::Utilities::getLatitudeFromTile(getZoom(), getYTile() + fy);
}

float OsmAndMapView::getRotatedMapLonForPoint(float x, float y) const{
    float dx = x - getCenterPointX();
    float dy = y - getCenterPointY();
    float fx = calcDiffTileX(dx, dy);
    return OsmAnd::Utilities::getLongitudeFromTile(getZoom(), getXTile() + fx);
}


PointI OsmAndMapView::getPixelPointForLatLon(double latitude, double longitude) const{
    return getPixelPoint((int32_t)OsmAnd::Utilities::get31TileNumberX(longitude),
                (int32_t) OsmAnd::Utilities::get31TileNumberY(latitude));
}

void OsmAndMapView::addLayer(std::shared_ptr<OsmAndMapDataLayer> layer) {
    layer->attach(this);
    layers.push_back(layer);
}

PointI OsmAndMapView::getPixelPoint(int32_t x31, int32_t y31) const{
    int cx = getCenterPointX();
    int cy = getCenterPointY();
    auto pw = OsmAnd::Utilities::getPowZoom(31 - getZoom());
    float diffxTile = x31 / pw - getXTile();
    float diffyTile = y31 / pw - getYTile();
    int newX = (int) (calcDiffPixelX(diffxTile, diffyTile) + cx);
    int newY = (int) (calcDiffPixelY(diffxTile, diffyTile) + cy);
    return PointI(newX, newY);
}

bool OsmAndMapView::isPoint31OnTheRotatedMap(int32_t x31, int32_t y31) const{
    PointI p = getPixelPoint(x31, y31);
    if (p.x >= 0 && p.x <= getWidth() && p.y >= 0 && p.y <= getHeight()) {
        return true;
    }
    return false;
}

bool OsmAndMapView::isLatLonPointOnTheRotatedMap(double latitude, double longitude) const{
    PointI p = getPixelPointForLatLon(latitude, longitude);
    if (p.x >= 0 && p.x <= getWidth() && p.y >= 0 && p.y <= getHeight()) {
        return true;
    }
    return false;
}

void OsmAndMapView::moveTo(float dx, float dy) {
    float fy = calcDiffTileY(dx, dy);
    float fx = calcDiffTileX(dx, dy);
    this->latitude = OsmAnd::Utilities::getLatitudeFromTile(getZoom(), getYTile() + fy);
    this->longitude = OsmAnd::Utilities::getLongitudeFromTile(getZoom(), getXTile() + fx);
    updateLayersViewport();
}

}
