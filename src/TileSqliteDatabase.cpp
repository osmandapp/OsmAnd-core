#include "TileSqliteDatabase.h"

#include "TileSqliteDatabase_P.h"

OsmAnd::TileSqliteDatabase::TileSqliteDatabase()
    : _p(new TileSqliteDatabase_P(this))
{
}

OsmAnd::TileSqliteDatabase::TileSqliteDatabase(
    QString filename_
)
    : _p(new TileSqliteDatabase_P(this))
    , filename(qMove(filename_))
{
}

OsmAnd::TileSqliteDatabase::~TileSqliteDatabase()
{
}

bool OsmAnd::TileSqliteDatabase::isOpened() const
{
    return _p->isOpened();
}

bool OsmAnd::TileSqliteDatabase::open()
{
    return _p->open();
}

bool OsmAnd::TileSqliteDatabase::close(bool compact /* = true */)
{
    return _p->close(compact);
}

bool OsmAnd::TileSqliteDatabase::isTileTimeSupported() const
{
    return _p->isTileTimeSupported();
}

bool OsmAnd::TileSqliteDatabase::shouldStoreTime() const
{
    return _p->shouldStoreTime();
}

bool OsmAnd::TileSqliteDatabase::hasTileTimeColumn() const
{
    return _p->hasTileTimeColumn();
}

void OsmAnd::TileSqliteDatabase::enableTileTimeSupportIfNeeded()
{
    _p->enableTileTimeSupportIfNeeded();
}

OsmAnd::ZoomLevel OsmAnd::TileSqliteDatabase::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::TileSqliteDatabase::getMaxZoom() const
{
    return _p->getMaxZoom();
}

bool OsmAnd::TileSqliteDatabase::recomputeMinMaxZoom()
{
    return _p->recomputeMinMaxZoom();
}

OsmAnd::AreaI OsmAnd::TileSqliteDatabase::getBBox31() const
{
    return _p->getBBox31();
}

bool OsmAnd::TileSqliteDatabase::recomputeBBox31(AreaI* pOutBBox31 /* = nullptr */)
{
    return _p->recomputeBBox31(pOutBBox31);
}

OsmAnd::AreaI OsmAnd::TileSqliteDatabase::getBBox31(ZoomLevel zoom) const
{
    return _p->getBBox31(zoom);
}

bool OsmAnd::TileSqliteDatabase::recomputeBBox31(ZoomLevel zoom, AreaI* pOutBBox31 /* = nullptr */)
{
    return _p->recomputeBBox31(zoom, pOutBBox31);
}

std::array<OsmAnd::AreaI, OsmAnd::ZoomLevelsCount> OsmAnd::TileSqliteDatabase::getBBoxes31() const
{
    return _p->getBBoxes31();
}

bool OsmAnd::TileSqliteDatabase::recomputeBBoxes31(
    std::array<OsmAnd::AreaI, OsmAnd::ZoomLevelsCount>* pOutBBoxes31 /* = nullptr */,
    AreaI* pOutBBox31 /* = nullptr */)
{
    return _p->recomputeBBoxes31(pOutBBoxes31, pOutBBox31);
}

bool OsmAnd::TileSqliteDatabase::obtainMeta(OsmAnd::TileSqliteDatabase::Meta& outMeta) const
{
    return _p->obtainMeta(outMeta);
}

bool OsmAnd::TileSqliteDatabase::storeMeta(const OsmAnd::TileSqliteDatabase::Meta& meta)
{
    return _p->storeMeta(meta);
}

bool OsmAnd::TileSqliteDatabase::containsTileData(OsmAnd::TileId tileId, OsmAnd::ZoomLevel zoom) const
{
    return _p->containsTileData(tileId, zoom);
}

bool OsmAnd::TileSqliteDatabase::obtainTileTime(
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom,
    int64_t& outTime) const
{
    return _p->obtainTileTime(tileId, zoom, outTime);
}

bool OsmAnd::TileSqliteDatabase::obtainTileData(
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom,
    QByteArray& outData,
    int64_t* pOutTime /* = nullptr*/) const
{
    return _p->obtainTileData(tileId, zoom, outData, pOutTime);
}

bool OsmAnd::TileSqliteDatabase::storeTileData(
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom,
    const QByteArray& data,
    int64_t time /* = 0*/)
{
    return _p->storeTileData(tileId, zoom, data, time);
}

bool OsmAnd::TileSqliteDatabase::removeTileData(
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom)
{
    return _p->removeTileData(tileId, zoom);
}

bool OsmAnd::TileSqliteDatabase::removeTilesData()
{
    return _p->removeTilesData();
}

bool OsmAnd::TileSqliteDatabase::removeTilesData(ZoomLevel zoom)
{
    return _p->removeTilesData(zoom);
}

bool OsmAnd::TileSqliteDatabase::removeTilesData(AreaI bbox31, bool strict /* = true */ )
{
    return _p->removeTilesData(bbox31, strict);
}

bool OsmAnd::TileSqliteDatabase::removeTilesData(AreaI bbox31, ZoomLevel zoom, bool strict /* = true */)
{
    return _p->removeTilesData(bbox31, zoom, strict);
}

bool OsmAnd::TileSqliteDatabase::compact()
{
    return _p->compact();
}

OsmAnd::TileSqliteDatabase::Meta::Meta()
{
}

OsmAnd::TileSqliteDatabase::Meta::~Meta()
{
}

const QString OsmAnd::TileSqliteDatabase::Meta::TITLE(QStringLiteral("title"));

QString OsmAnd::TileSqliteDatabase::Meta::getTitle(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itTitle = values.constFind(TITLE);
    if (itTitle == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itTitle->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setTitle(QString title)
{
    values.insert(TITLE, QVariant(qMove(title)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::RULE(QStringLiteral("rule"));

QString OsmAnd::TileSqliteDatabase::Meta::getRule(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itRule = values.constFind(RULE);
    if (itRule == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itRule->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setRule(QString rule)
{
    values.insert(RULE, QVariant(qMove(rule)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::REFERER(QStringLiteral("referer"));

QString OsmAnd::TileSqliteDatabase::Meta::getReferer(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itReferer = values.constFind(REFERER);
    if (itReferer == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itReferer->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setReferer(QString referer)
{
    values.insert(REFERER, QVariant(qMove(referer)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::RANDOMS(QStringLiteral("randoms"));

QString OsmAnd::TileSqliteDatabase::Meta::getRandoms(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itRandoms = values.constFind(RANDOMS);
    if (itRandoms == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itRandoms->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setRandoms(QString randoms)
{
    values.insert(RANDOMS, QVariant(qMove(randoms)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::URL(QStringLiteral("url"));

QString OsmAnd::TileSqliteDatabase::Meta::getUrl(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itUrl = values.constFind(URL);
    if (itUrl == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itUrl->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setUrl(QString url)
{
    values.insert(URL, QVariant(qMove(url)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::MIN_ZOOM(QStringLiteral("minzoom"));

int64_t OsmAnd::TileSqliteDatabase::Meta::getMinZoom(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itMinZoom = values.constFind(MIN_ZOOM);
    if (itMinZoom == values.cend())
        return std::numeric_limits<int64_t>::min();

    bool ok = false;
    const auto minZoom = itMinZoom->toLongLong(&ok);

    if (!ok)
        return std::numeric_limits<int64_t>::min();

    if (outOk)
        *outOk = true;
    return minZoom;
}

void OsmAnd::TileSqliteDatabase::Meta::setMinZoom(int64_t minZoom)
{
    values.insert(MIN_ZOOM, QVariant(static_cast<qint64>(minZoom)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::MAX_ZOOM(QStringLiteral("maxzoom"));

int64_t OsmAnd::TileSqliteDatabase::Meta::getMaxZoom(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itMaxZoom = values.constFind(MAX_ZOOM);
    if (itMaxZoom == values.cend())
        return std::numeric_limits<int64_t>::max();

    bool ok = false;
    const auto maxZoom = itMaxZoom->toLongLong(&ok);

    if (!ok)
        return std::numeric_limits<int64_t>::max();

    if (outOk)
        *outOk = true;
    return maxZoom;
}

void OsmAnd::TileSqliteDatabase::Meta::setMaxZoom(int64_t maxZoom)
{
    values.insert(MAX_ZOOM, QVariant(static_cast<qint64>(maxZoom)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::ELLIPSOID(QStringLiteral("ellipsoid"));

int64_t OsmAnd::TileSqliteDatabase::Meta::getEllipsoid(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itEllipsoid = values.constFind(ELLIPSOID);
    if (itEllipsoid == values.cend())
        return std::numeric_limits<int64_t>::min();

    bool ok = false;
    const auto ellipsoid = itEllipsoid->toLongLong(&ok);

    if (!ok)
        return std::numeric_limits<int64_t>::min();

    if (outOk)
        *outOk = true;
    return ellipsoid;
}

void OsmAnd::TileSqliteDatabase::Meta::setEllipsoid(int64_t ellipsoid)
{
    values.insert(ELLIPSOID, QVariant(static_cast<qint64>(ellipsoid)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::INVERTED_Y(QStringLiteral("inverted_y"));

int64_t OsmAnd::TileSqliteDatabase::Meta::getInvertedY(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itInvertedY = values.constFind(INVERTED_Y);
    if (itInvertedY == values.cend())
        return std::numeric_limits<int64_t>::min();

    bool ok = false;
    const auto invertedY = itInvertedY->toLongLong(&ok);

    if (!ok)
        return std::numeric_limits<int64_t>::min();

    if (outOk)
        *outOk = true;
    return invertedY;
}

void OsmAnd::TileSqliteDatabase::Meta::setInvertedY(int64_t invertedY)
{
    values.insert(INVERTED_Y, QVariant(static_cast<qint64>(invertedY)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::TIME_COLUMN(QStringLiteral("timecolumn"));

QString OsmAnd::TileSqliteDatabase::Meta::getTimeColumn(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itTimeColumn = values.constFind(TIME_COLUMN);
    if (itTimeColumn == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itTimeColumn->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setTimeColumn(QString timeColumn)
{
    values.insert(TIME_COLUMN, QVariant(qMove(timeColumn)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::EXPIRE_MINUTES(QStringLiteral("expireminutes"));

int64_t OsmAnd::TileSqliteDatabase::Meta::getExpireMinutes(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto iteExpireMinutes = values.constFind(EXPIRE_MINUTES);
    if (iteExpireMinutes == values.cend())
        return std::numeric_limits<int64_t>::min();

    bool ok = false;
    const auto expireMinutes = iteExpireMinutes->toLongLong(&ok);

    if (!ok)
        return std::numeric_limits<int64_t>::min();

    if (outOk)
        *outOk = true;
    return expireMinutes;
}

void OsmAnd::TileSqliteDatabase::Meta::setExpireMinutes(int64_t expireMinutes)
{
    values.insert(EXPIRE_MINUTES, QVariant(static_cast<qint64>(expireMinutes)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::TILE_NUMBERING(QStringLiteral("tilenumbering"));

QString OsmAnd::TileSqliteDatabase::Meta::getTileNumbering(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itTileNumbering = values.constFind(TILE_NUMBERING);
    if (itTileNumbering == values.cend())
        return QString();

    if (outOk)
        *outOk = true;
    return itTileNumbering->toString();
}

void OsmAnd::TileSqliteDatabase::Meta::setTileNumbering(QString tileNumbering)
{
    values.insert(TILE_NUMBERING, QVariant(qMove(tileNumbering)));
}

const QString OsmAnd::TileSqliteDatabase::Meta::TILE_SIZE(QStringLiteral("tilesize"));

int64_t OsmAnd::TileSqliteDatabase::Meta::getTileSize(bool* outOk /* = nullptr*/) const
{
    if (outOk)
        *outOk = false;

    const auto itTileSize = values.constFind(TILE_SIZE);
    if (itTileSize == values.cend())
        return std::numeric_limits<int64_t>::min();

    bool ok = false;
    const auto tileSize = itTileSize->toLongLong(&ok);

    if (!ok)
        return std::numeric_limits<int64_t>::min();

    if (outOk)
        *outOk = true;
    return tileSize;
}

void OsmAnd::TileSqliteDatabase::Meta::setTileSize(int64_t tileSize)
{
    values.insert(TILE_SIZE, QVariant(static_cast<qint64>(tileSize)));
}
