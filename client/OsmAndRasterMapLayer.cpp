#include "OsmAndRasterMapLayer.h"

#include <SkImageDecoder.h>
#include <SkBitmapFactory.h>
#include <SkCanvas.h>
#include <OsmAndCore/Logging.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

class OsmAnd::RasterMapDownloader : public OsmAnd::OsmAndTask {
    int x, y, z;
    std::shared_ptr<MapTileSource> tileSource;
    const QString& toFile;
public:
    RasterMapDownloader(std::shared_ptr<MapTileSource> tileSource,
                        int x, int y, int z, const QString& toFile) :
        x(x), y(y), z(z), tileSource(tileSource), toFile(toFile) {}

    virtual void run()  {
        const QString& url = tileSource->getUrlToLoad(x, y, z);
        if(!url.isEmpty()) {
            QNetworkAccessManager mgr;
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "Download tile %s", url.toStdString().c_str());
            QUrl qurl(url);
            QNetworkRequest req(qurl);
            QNetworkReply* rep = mgr.get(req);
            QByteArray data = rep->readAll();
            QFile f(toFile);
            f.write(data);
        }


    }

    virtual void onPostExecute() {
        // TODO signal to refresh map
    }

    virtual const QString& getDescription() {
        return "Download tile "  + tileSource->getName() + " " +
                QString::number(z) +"/" + QString::number(x) +"/" + QString::number(y);
    }

    virtual ~RasterMapDownloader() {}


};

void OsmAnd::OsmAndRasterMapLayer::setTileSource(std::shared_ptr<MapTileSource> tileSource){
    this->tileSource = tileSource;
    clearCache();
}

void OsmAnd::OsmAndRasterMapLayer::clearCache() {
    cache.clear();
    cacheValues.clear();
}

void OsmAnd::OsmAndRasterMapLayer::updateViewport(OsmAnd::OsmAndMapView *view) {
    QRectF r = view->getTileRect();
    //    QString tileSource = app->getSettings()->TILE_SOURCE.get().toString();
    renderRaster(view, app->getSettings()->APPLICATION_DIRECTORY.get().toString());
    double z = OsmAnd::Utilities::getPowZoom(31 - view->getZoom());
    topLeft = new MapPoint(view,(int32_t) (z * r.left()), (int32_t) (z * r.top()));
    bottomRight = new MapPoint(view,(int32_t) (z * r.right()), (int32_t) (z * r.bottom()));
    SkBitmap* swap = doubleBuffer;
    this->doubleBuffer = img;
    this->img = swap;
}



SkBitmap* OsmAnd::OsmAndRasterMapLayer::getBitmap(MapPoint* topLeft, MapPoint* bottomRight) {
    if ( img == nullptr) {
        return img;
    }
    *topLeft = *this->topLeft;
    *bottomRight = *this->bottomRight;
    return img;
}


std::shared_ptr<SkBitmap>  OsmAnd::OsmAndRasterMapLayer::loadImage(int x, int y, int z, QString &file) {
    auto i = cache.find(file);
    if(i == cache.end()) {
        std::shared_ptr<SkBitmap> tileBitmap = std::shared_ptr<SkBitmap>(new SkBitmap());
        //TODO: JPEG is badly supported! At the moment it needs sdcard to be present (sic). Patch that
        if(!SkImageDecoder::DecodeFile(file.toStdString().c_str(), tileBitmap.get(), SkBitmap::kARGB_8888_Config,SkImageDecoder::kDecodePixels_Mode))
        {
            if(!requestedToDownload.contains(file)) {
                requestedToDownload.insert(file);
                app->submitTask(QString("downloadTiles"), std::shared_ptr<OsmAnd::OsmAndTask>(new RasterMapDownloader(tileSource, x, y, z, file)));
            }

            return nullptr;
        }
        cacheValues[file] = 3;
        return  (cache[file] = tileBitmap);
    } else {
        return *i;
    }

}

void OsmAnd::OsmAndRasterMapLayer::renderRaster(OsmAnd::OsmAndMapView *view, const QString& appDir)
{
    if(doubleBuffer == nullptr || ( doubleBuffer->width() != view->getWidth() || view->getHeight() != img->height())) {
        if(doubleBuffer != nullptr) {
            delete (uchar*)doubleBuffer->getPixels();
            delete img;
        }
        doubleBuffer = new SkBitmap();
        doubleBuffer->setConfig(SkBitmap::kARGB_8888_Config, view->getWidth(), view->getHeight());
        uchar* bitmapData = new uchar[doubleBuffer->getSize()];
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Allocated %d bytes at %p", doubleBuffer->getSize(), bitmapData);
        doubleBuffer->setPixels(bitmapData);
    }
    QRectF ts =  view->getTileRect();
    int left = floor(ts.x());
    int top = floor(ts.y());
    int width  = ceil(ts.x() + ts.width()) - left;
    int height  = ceil(ts.y() + ts.height()) - top;

    float tileX = view->getXTile();
    float tileY = view->getYTile();
    float w = view->getCenterPointX();
    float h = view->getCenterPointY();
    float ftileSize = view->getTileSize();
    int z = view->getZoom();

    QVector<QVector<std::shared_ptr<SkBitmap> > > images;
    if(tileSource) {
        QString base =  appDir + "/tiles/" + tileSource->getName() +"/" + QString::number(z)+"/";
        images.resize(width);
        for (int i = 0; i < width; i++) {
            images[i].resize(height);
            for (int j = 0; j < height; j++) {
                int x = left + i;
                int y = top+j;
                QString s = base +  QString::number(x) +"/"+QString::number(y)+""+tileSource->getTileFormat()+".tile";
                images[i][j] = loadImage(x, y, z, s);
                cacheValues[s] += 3;
            }
        }
    }
    SkCanvas cvs(*doubleBuffer);
    cvs.clear(SK_ColorTRANSPARENT);
    SkPaint pntImage;
    pntImage.setFilterBitmap(true);
    pntImage.setDither(true);
    cvs.translate(view->getCenterPointX(), view->getCenterPointY());
    cvs.rotate(view->getRotate());

    cvs.translate(-view->getCenterPointX(), -view->getCenterPointY());
    for (int  i = 0; i < images.size(); i++) {
        for (int j = 0; j < images[i].size(); j++) {
            float x1 = (left + i - tileX) * ftileSize + w;
            float y1 = (top + j - tileY) * ftileSize + h;
            if(images[i][j] != nullptr) {
                cvs.drawBitmap(*images[i][j], x1, y1, &pntImage);
            }
        }
    }
    // ugly fix
    uint32_t* data = (uint32_t*) doubleBuffer->getPixels();
    int sz = doubleBuffer->getSize() / 4;
    for( int i = 0; i< sz ; i++) {
        data[i] = (data[i] & 0xff00ff00) |  ((data[i] & 0x00ff0000) >> 16 ) |  ((data[i] & 0x000000ff) << 16 );
    }
    // end

    // clear cache
    clearCachePartially();
}

void OsmAnd::OsmAndRasterMapLayer::clearCachePartially() {
    QMap<QString, int>::iterator cvsIterator = cacheValues.begin();
    while(cvsIterator != cacheValues.end()) {
        (*cvsIterator) /= 2;
        if((*cvsIterator) <= 1) {
            QString s = cvsIterator.key();
            cvsIterator = cacheValues.erase(cvsIterator);
            cache.remove(s);
        } else {
            cvsIterator++;
        }
    }
}
