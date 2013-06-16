#ifndef MAPTILESOURCE_H
#define MAPTILESOURCE_H

#include <OsmAndCore.h>
#include <OsmAndUtilities.h>
#include <QString>


namespace OsmAnd {


class OSMAND_CORE_API MapTileSource
{
public:
    virtual int getMaximumZoomSupported() = 0;

    virtual const QString& getName() = 0;

    virtual int getTileSize() = 0;

    virtual const QString& getUrlToLoad(int x, int y, int zoom)=0;

    virtual int getMinimumZoomSupported()= 0;

    virtual const QString& getTileFormat() = 0;

    virtual int getBitDensity() {return 8;}

    virtual bool isEllipticYTile() {return false;}

    virtual bool couldBeDownloadedFromInternet() = 0;

    const static std::shared_ptr<MapTileSource> kMapnik;
    const static std::shared_ptr<MapTileSource> kCyclemap;
};

class OSMAND_CORE_API TileSourceTemplate : public MapTileSource {

public:
    int maxZoom;
    int minZoom;
    QString name;
    int tileSize;
    QString urlToLoad;
    QString ext;
    int avgSize;
    int bitDensity;
    bool ellipticYTile;
    QString rule;
    bool isRuleAcceptable;

    TileSourceTemplate(QString name, QString urlToLoad, QString ext, int maxZoom, int minZoom, int tileSize, int bitDensity,
                       int avgSize) {
        isRuleAcceptable = true;
        this->maxZoom = maxZoom;
        this->minZoom = minZoom;
        this->name = name;
        this->tileSize = tileSize;
        this->urlToLoad = urlToLoad;
        this->ext = ext;
        this->avgSize = avgSize;
        this->bitDensity = bitDensity;
    }


    virtual int getMaximumZoomSupported() {
        return maxZoom;
    }

    virtual int getMinimumZoomSupported() {
        return minZoom;
    }

    virtual  const QString& getName() {
        return name;
    }

    virtual int getTileSize() {
        return tileSize;
    }

    virtual const QString& getTileFormat() {
        return ext;
    }


    virtual const QString& getUrlToLoad(int x, int y, int zoom) {
        // use int to QString not format numbers! (non-nls)
        if (urlToLoad == "") {
            return "";
        }
        QString res = urlToLoad;
        res.replace("${x}", QString::number(x))
        .replace("${y}", QString::number(y))
        .replace("${z}", QString::number(zoom));
        return res;
    }

    virtual QString getUrlTemplate() {
        return urlToLoad;
    }

    virtual bool couldBeDownloadedFromInternet() {
        return urlToLoad != "";
    }
};

}

#endif // MAPTILESOURCE_H
