#include "TileSqliteDatabase_P.h"
#include "TileSqliteDatabase.h"

#include "ignore_warnings_on_external_includes.h"
#include <QDateTime>
#include <QTime>
#include "restore_internal_warnings.h"

#include "Logging.h"

OsmAnd::TileSqliteDatabase_P::TileSqliteDatabase_P(
    TileSqliteDatabase* owner_
)
    : _isOpened(0)
    , owner(owner_)
{
}

OsmAnd::TileSqliteDatabase_P::~TileSqliteDatabase_P()
{
    close();
}

void OsmAnd::TileSqliteDatabase_P::resetCachedInfo()
{
    _cachedInvertedZoom.storeRelease(std::numeric_limits<int>::min());
    _cachedInvertedY.storeRelease(std::numeric_limits<int>::min());
    _cachedMinZoom.storeRelease(ZoomLevel::InvalidZoomLevel);
    _cachedMaxZoom.storeRelease(ZoomLevel::InvalidZoomLevel);
    _cachedIsTileTimeSupported.storeRelease(std::numeric_limits<int>::min());

    {
        QWriteLocker scopedLocker(&_cachedBboxes31Lock);
        _cachedBbox31 = AreaI::negative();
        _cachedBboxes31.fill(AreaI::negative());
    }
}

bool OsmAnd::TileSqliteDatabase_P::isOpened() const
{
    return _isOpened.loadAcquire() != 0;
}

bool OsmAnd::TileSqliteDatabase_P::open()
{
    if (!isOpened())
    {
        QWriteLocker scopedLocker(&_lock);

        if (isOpened())
            return true;

        int res;
        const auto filename = owner->filename.isEmpty() ? QStringLiteral(":memory:") : owner->filename;

        sqlite3* pDatabase = nullptr;
        res = sqlite3_open16(filename.utf16(), &pDatabase);
        const std::shared_ptr<sqlite3> database(pDatabase, sqlite3_close);

        if (res != SQLITE_OK)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to load '%s': %s (%s)",
                qPrintable(filename),
                database ? sqlite3_errmsg(database.get()) : "N/A",
                sqlite3_errstr(res));

            return false;
        }

        const auto meta = readMeta(database);

        if (!execStatement(database, QStringLiteral(
            "CREATE TABLE IF NOT EXISTS tiles("
            "   x INTEGER NOT NULL,"
            "   y INTEGER NOT NULL,"
            "   z INTEGER NOT NULL,"
            "   image BLOB,"
            "   PRIMARY KEY (x, y, z)"
            ")")))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to create tiles table: %s",
                sqlite3_errmsg(database.get()));

            return false;
        }

        if (!execStatement(database, QStringLiteral("CREATE INDEX IF NOT EXISTS tiles_x_index ON tiles(x)")))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to create index on tiles(x): %s",
                sqlite3_errmsg(database.get()));

            return false;
        }

        if (!execStatement(database, QStringLiteral("CREATE INDEX IF NOT EXISTS tiles_y_index ON tiles(y)")))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to create index on tiles(y): %s",
                sqlite3_errmsg(database.get()));

            return false;
        }

        if (!execStatement(database, QStringLiteral("CREATE INDEX IF NOT EXISTS tiles_z_index ON tiles(z)")))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to create index on tiles(z): %s",
                sqlite3_errmsg(database.get()));

            return false;
        }

        resetCachedInfo();
        _database = database;
        _meta = meta;
        _isOpened.storeRelease(1);
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::close(bool compact /* = true */)
{
    if (isOpened())
    {
        QWriteLocker scopedLocker(&_lock);

        if (!isOpened())
            return true;

        if (_meta && !writeMeta(_database, *_meta))
        {
            return false;
        }

        if (compact && _meta && !_meta->getUrl().isEmpty() && !vacuum(_database))
        {
            return false;
        }

        resetCachedInfo();
        _database.reset();
        _meta.reset();
        _isOpened.storeRelease(0);
    }

    return true;
}

int OsmAnd::TileSqliteDatabase_P::getInvertedZoomValue() const
{
    int invertedZoomValue;
    if ((invertedZoomValue = _cachedInvertedZoom.loadAcquire()) >= 0)
    {
        return invertedZoomValue;
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        _cachedInvertedZoom.storeRelease(0);
        return 0;
    }

    bool ok = false;
    const auto tileNumberingValue = meta.getTileNumbering(&ok);
    if (!ok)
    {
        _cachedInvertedZoom.storeRelease(0);
        return 0;
    }

    if (tileNumberingValue.isEmpty() || tileNumberingValue == QStringLiteral("OSM"))
    {
        invertedZoomValue = 0;
    }
    else if (tileNumberingValue == QStringLiteral("BigPlanet"))
    {
        invertedZoomValue = 17;
    }
    else
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "Unsupported tile numbering '%s'",
            qPrintable(tileNumberingValue));
        _cachedInvertedZoom.storeRelease(0);
        return 0;
    }

    _cachedInvertedZoom.storeRelease(invertedZoomValue);
    return invertedZoomValue;
}
                                                                                         
bool OsmAnd::TileSqliteDatabase_P::isInvertedY() const
{
    int invertedY;
    if ((invertedY = _cachedInvertedY.loadAcquire()) >= 0)
    {
        return invertedY > 0;
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        _cachedInvertedY.storeRelease(0);
        return false;
    }

    bool ok = false;
    invertedY = meta.getInvertedY(&ok);
    if (!ok)
    {
        _cachedInvertedY.storeRelease(0);
        return false;
    }

    _cachedInvertedY.storeRelease(invertedY);
    return invertedY > 0;
}

bool OsmAnd::TileSqliteDatabase_P::isTileTimeSupported() const
{
    int isSupported;
    if ((isSupported = _cachedIsTileTimeSupported.loadAcquire()) >= 0)
    {
        return isSupported > 0;
    }

    if (!isOpened())
    {
        return false;
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        _cachedIsTileTimeSupported.storeRelease(0);
        return false;
    }

    bool ok = false;
    const auto timeColumn = meta.getTimeColumn(&ok);
    if (!ok)
    {
        _cachedIsTileTimeSupported.storeRelease(0);
        return false;
    }

    isSupported = QString::compare(timeColumn, QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
    isSupported = isSupported && execStatement(_database, QStringLiteral("SELECT time FROM tiles LIMIT 1"));
    
    _cachedIsTileTimeSupported.storeRelease(isSupported);
    return isSupported > 0;
}

bool OsmAnd::TileSqliteDatabase_P::hasTimeColumn() const
{
    if (!isOpened())
    {
        return false;
    }
    return execStatement(_database, QStringLiteral("SELECT time FROM tiles LIMIT 1"));
 }

bool OsmAnd::TileSqliteDatabase_P::enableTileTimeSupport(bool force /* = false */)
{
    if (!isOpened())
    {
        return false;
    }

    if (isTileTimeSupported() && !force)
    {
        return true;
    }

    {
        QReadLocker scopedLocker(&_lock);

        if (!execStatement(_database, QStringLiteral("SELECT time FROM tiles LIMIT 1")))
        {
            if (!execStatement(_database, QStringLiteral("ALTER TABLE tiles ADD COLUMN time INTEGER")))
            {
                LogPrintf(
                    LogSeverityLevel::Error,
                    "Failed to add time column: %s",
                    sqlite3_errmsg(_database.get()));
                return false;
            }
        }
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        return false;
    }

    meta.setTimeColumn(QStringLiteral("yes"));

    if (!storeMeta(meta))
    {
        return false;
    }

    _cachedIsTileTimeSupported.storeRelease(1);

    return true;
}

OsmAnd::ZoomLevel OsmAnd::TileSqliteDatabase_P::getMinZoom() const
{
    ZoomLevel minZoom;
    if ((minZoom = static_cast<ZoomLevel>(_cachedMinZoom.loadAcquire())) != ZoomLevel::InvalidZoomLevel)
    {
        return minZoom;
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        _cachedMinZoom.storeRelease(ZoomLevel::MinZoomLevel);
        return ZoomLevel::MinZoomLevel;
    }

    bool ok = false;
    auto minZoomValue = meta.getMinZoom(&ok);
    if (!ok)
    {
        _cachedMinZoom.storeRelease(ZoomLevel::MinZoomLevel);
        return ZoomLevel::MinZoomLevel;
    }
    auto maxZoomValue = meta.getMaxZoom(&ok);
    if (!ok)
    {
        _cachedMinZoom.storeRelease(ZoomLevel::MinZoomLevel);
        return ZoomLevel::MinZoomLevel;
    }

    int invertedZoomValue;
    if ((invertedZoomValue = getInvertedZoomValue()) > 0)
    {
        minZoomValue = invertedZoomValue - maxZoomValue;
    }

    if (minZoomValue < ZoomLevel::MinZoomLevel || minZoomValue > ZoomLevel::MaxZoomLevel)
    {
        _cachedMinZoom.storeRelease(ZoomLevel::MinZoomLevel);
        return ZoomLevel::MinZoomLevel;
    }

    _cachedMinZoom.storeRelease(minZoomValue);
    return static_cast<ZoomLevel>(minZoomValue);
}

OsmAnd::ZoomLevel OsmAnd::TileSqliteDatabase_P::getMaxZoom() const
{
    ZoomLevel maxZoom;
    if ((maxZoom = static_cast<ZoomLevel>(_cachedMaxZoom.loadAcquire())) != ZoomLevel::InvalidZoomLevel)
    {
        return maxZoom;
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        _cachedMaxZoom.storeRelease(ZoomLevel::MaxZoomLevel);
        return ZoomLevel::MaxZoomLevel;
    }

    bool ok = false;
    auto maxZoomValue = meta.getMaxZoom(&ok);
    if (!ok)
    {
        _cachedMaxZoom.storeRelease(ZoomLevel::MaxZoomLevel);
        return ZoomLevel::MaxZoomLevel;
    }
    auto minZoomValue = meta.getMinZoom(&ok);
    if (!ok)
    {
        _cachedMaxZoom.storeRelease(ZoomLevel::MaxZoomLevel);
        return ZoomLevel::MaxZoomLevel;
    }

    int invertedZoomValue;
    if ((invertedZoomValue = getInvertedZoomValue()) > 0)
    {
        maxZoomValue = invertedZoomValue - minZoomValue;
    }

    if (maxZoomValue < ZoomLevel::MinZoomLevel || maxZoomValue > ZoomLevel::MaxZoomLevel)
    {
        _cachedMaxZoom.storeRelease(ZoomLevel::MaxZoomLevel);
        return ZoomLevel::MaxZoomLevel;
    }

    _cachedMaxZoom.storeRelease(maxZoomValue);
    return static_cast<ZoomLevel>(maxZoomValue);
}

bool OsmAnd::TileSqliteDatabase_P::recomputeMinMaxZoom()
{
    auto minZoom = ZoomLevel::InvalidZoomLevel;
    auto maxZoom = ZoomLevel::InvalidZoomLevel;

    {
        QReadLocker scopedLocker(&_lock);

        int res;

        const auto statement = prepareStatement(_database, QStringLiteral("SELECT MIN(z), MAX(z) FROM tiles"));
        if (!statement || (res = stepStatement(statement)) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query min-max zoom: %s",
                sqlite3_errmsg(_database.get()));

            return false;
        }

        if (res > 0)
        {
            bool ok = false;

            const auto minZoomValue = readStatementValue(statement, 0).toLongLong(&ok);
            if (!ok || minZoomValue < ZoomLevel::MinZoomLevel || minZoomValue > ZoomLevel::MaxZoomLevel)
            {
                return false;
            }
            minZoom = static_cast<ZoomLevel>(minZoomValue);

            const auto maxZoomValue = readStatementValue(statement, 1).toLongLong(&ok);
            if (!ok || maxZoomValue < ZoomLevel::MinZoomLevel || maxZoomValue > ZoomLevel::MaxZoomLevel)
            {
                return false;
            }
            maxZoom = static_cast<ZoomLevel>(maxZoomValue);
        }
        else
        {
            minZoom = ZoomLevel::MinZoomLevel;
            maxZoom = ZoomLevel::MaxZoomLevel;
        }
    }

    Meta meta;
    if (!obtainMeta(meta))
    {
        return false;
    }

    meta.setMinZoom(minZoom);
    meta.setMaxZoom(maxZoom);

    if (!storeMeta(meta))
    {
        return false;
    }

    _cachedMinZoom.storeRelease(minZoom);
    _cachedMaxZoom.storeRelease(maxZoom);

    return true;
}

OsmAnd::AreaI OsmAnd::TileSqliteDatabase_P::getBBox31() const
{
    QReadLocker scopedLocker(&_cachedBboxes31Lock);

    return _cachedBbox31;
}

bool OsmAnd::TileSqliteDatabase_P::recomputeBBox31(AreaI* pOutBBox31 /* = nullptr */)
{
    return recomputeBBoxes31(nullptr, pOutBBox31);
}

OsmAnd::AreaI OsmAnd::TileSqliteDatabase_P::getBBox31(ZoomLevel zoom) const
{
    QReadLocker scopedLocker(&_cachedBboxes31Lock);

    return _cachedBboxes31[zoom];
}

bool OsmAnd::TileSqliteDatabase_P::recomputeBBox31(ZoomLevel zoom, AreaI* pOutBBox31 /* = nullptr */)
{
    AreaI bbox31 = AreaI::negative();

    {
        QReadLocker scopedLocker(&_lock);

        int res;

        const auto statement = prepareStatement(_database, QStringLiteral("SELECT MIN(x), MAX(x), MIN(y), MAX(y) FROM tiles WHERE z=:z"));
        if (!statement || !configureStatement(statement, zoom) || (res = stepStatement(statement)) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query bbox@%d: %s",
                zoom,
                sqlite3_errmsg(_database.get()));

            return false;
        }

        if (res > 0)
        {
            bool ok = false;

            auto minX = readStatementValue(statement, 0).toLongLong(&ok);
            if (!ok || minX < 0)
            {
                return false;
            }

            auto maxX = readStatementValue(statement, 1).toLongLong(&ok);
            if (!ok || maxX < 0)
            {
                return false;
            }

            auto minY = readStatementValue(statement, 2).toLongLong(&ok);
            if (!ok || minY < 0)
            {
                return false;
            }

            auto maxY = readStatementValue(statement, 3).toLongLong(&ok);
            if (!ok || maxY < 0)
            {
                return false;
            }

            if (isInvertedY())
            {
                minY = (1 << zoom) - 1 - minY;
                maxY = (1 << zoom) - 1 - maxY;
            }

            bbox31 = AreaI(
                minY << (31 - zoom),
                minX << (31 - zoom),
                ((maxY + 1) << (31 - zoom)) - 1,
                ((maxX + 1) << (31 - zoom)) - 1
            );
        }
    }

    {
        QWriteLocker scopedLocker(&_cachedBboxes31Lock);

        _cachedBboxes31[zoom] = bbox31;

        _cachedBbox31 = AreaI::negative();
        for (int zoom = getMinZoom(); zoom <= getMaxZoom(); zoom++)
        {
            const auto& zoomBbox31 = _cachedBboxes31[zoom];
            if (!zoomBbox31.isNegative())
            {
                _cachedBbox31.enlargeToInclude(zoomBbox31);
            }
        }
    }

    if (pOutBBox31)
    {
        *pOutBBox31 = bbox31;
    }

    return true;
}

std::array<OsmAnd::AreaI, OsmAnd::ZoomLevelsCount> OsmAnd::TileSqliteDatabase_P::getBBoxes31() const
{
    QReadLocker scopedLocker(&_cachedBboxes31Lock);

    return _cachedBboxes31;
}

bool OsmAnd::TileSqliteDatabase_P::recomputeBBoxes31(
    std::array<OsmAnd::AreaI, OsmAnd::ZoomLevelsCount>* pOutBBoxes31 /* = nullptr */,
    AreaI* pOutBBox31 /* = nullptr*/)
{
    std::array<AreaI, ZoomLevelsCount> bboxes31;
    bboxes31.fill(AreaI::negative());

    AreaI bbox31 = AreaI::negative();

    {
        QReadLocker scopedLocker(&_lock);

        int res;

        const auto statement = prepareStatement(_database, QStringLiteral("SELECT z, MIN(x), MAX(x), MIN(y), MAX(y) FROM tiles GROUP BY z"));
        if (!statement)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query bboxes grouped by zoom: %s",
                sqlite3_errmsg(_database.get()));

            return false;
        }

        while ((res = stepStatement(statement)) > 0)
        {
            bool ok = false;

            const auto zoomValue = readStatementValue(statement, 0).toLongLong(&ok);
            if (!ok || zoomValue < ZoomLevel::MinZoomLevel || zoomValue > ZoomLevel::MaxZoomLevel)
            {
                continue;
            }
            auto zoom = static_cast<ZoomLevel>(zoomValue);

            int invertedZoomValue;
            if ((invertedZoomValue = getInvertedZoomValue()) > 0)
            {
                zoom = static_cast<ZoomLevel>(invertedZoomValue - zoom);
            }

            auto minX = readStatementValue(statement, 1).toLongLong(&ok);
            if (!ok || minX < 0)
            {
                continue;
            }

            auto maxX = readStatementValue(statement, 2).toLongLong(&ok);
            if (!ok || maxX < 0)
            {
                continue;
            }

            auto minY = readStatementValue(statement, 3).toLongLong(&ok);
            if (!ok || minY < 0)
            {
                continue;
            }

            auto maxY = readStatementValue(statement, 4).toLongLong(&ok);
            if (!ok || maxY < 0)
            {
                continue;
            }

            if (isInvertedY())
            {
                minY = (1 << zoom) - 1 - minY;
                maxY = (1 << zoom) - 1 - maxY;
            }

            const auto zoomBbox31 = AreaI(
                minY << (31 - zoom),
                minX << (31 - zoom),
                ((maxY + 1) << (31 - zoom)) - 1,
                ((maxX + 1) << (31 - zoom)) - 1
            );

            bboxes31[zoom] = zoomBbox31;
            bbox31.enlargeToInclude(zoomBbox31);
        }

        if (res < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query bboxes grouped by zoom: %s",
                sqlite3_errmsg(_database.get()));

            return false;
        }
    }

    {
        QWriteLocker scopedLocker(&_cachedBboxes31Lock);

        _cachedBboxes31 = bboxes31;
        _cachedBbox31 = bbox31;
    }

    if (pOutBBoxes31)
    {
        *pOutBBoxes31 = bboxes31;
    }
    if (pOutBBox31)
    {
        *pOutBBox31 = bbox31;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::obtainMeta(OsmAnd::TileSqliteDatabase_P::Meta& outMeta) const
{
    if (!isOpened())
    {
        return false;
    }

    {
        QMutexLocker scopedLocker(&_metaLock);

        if (!_meta)
        {
            _meta = readMeta(_database);
        }
        if (!_meta)
        {
            return false;
        }

        outMeta = *_meta;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::storeMeta(const OsmAnd::TileSqliteDatabase_P::Meta& meta)
{
    if (!isOpened())
    {
        return false;
    }

    {
        QMutexLocker scopedLocker(&_metaLock);

        _meta = std::make_shared<Meta>(meta);

        if (!writeMeta(_database, meta))
        {
            return false;
        }
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::containsTileData(OsmAnd::TileId tileId, OsmAnd::ZoomLevel zoom) const
{
    if (!isOpened())
    {
        return false;
    }

    if (zoom < getMinZoom() || zoom > getMaxZoom())
    {
        return false;
    }

    {
        QReadLocker scopedLocker(&_lock);

        int res;

        const auto statement = prepareStatement(_database, QStringLiteral("EXISTS(SELECT 1 FROM tiles WHERE x=:x AND y=:y AND z=:z LIMIT 1)"));
        if (!statement || !configureStatement(statement, tileId, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }
        if ((res = stepStatement(statement)) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query for %dx%d@%d existence: %s",
                tileId.x,
                tileId.y,
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }

        return res > 0;
    }
}

bool OsmAnd::TileSqliteDatabase_P::obtainTileTime(
    TileId tileId,
    ZoomLevel zoom,
    int64_t& outTime) const
{
    if (!isOpened())
    {
        return false;
    }

    if (!isTileTimeSupported())
    {
        return false;
    }

    if (zoom < getMinZoom() || zoom > getMaxZoom())
    {
        return false;
    }

    {
        QReadLocker scopedLocker(&_lock);

        int res;

        const auto statement = prepareStatement(_database, QStringLiteral("SELECT time FROM tiles WHERE x=:x AND y=:y AND z=:z"));
        if (!statement || !configureStatement(statement, tileId, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }
        if ((res = stepStatement(statement)) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query for %dx%d@%d time: %s",
                tileId.x,
                tileId.y,
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }

        if (res > 0)
        {
            int64_t time = 0;
            const auto timeValue = readStatementValue(statement, 0);
            if (!timeValue.isNull())
            {
                bool ok = false;
                const auto timeLL = timeValue.toLongLong(&ok);
                if (!ok)
                {
                    return false;
                }
                time = static_cast<int64_t>(timeLL);
            }
            
            outTime = time;
            return true;
        }
    }

    return false;
}

bool OsmAnd::TileSqliteDatabase_P::obtainTileData(
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom,
    QByteArray& outData,
    int64_t* pOutTime /* = nullptr */) const
{
    if (!isOpened())
    {
        return false;
    }

    if (zoom < getMinZoom() || zoom > getMaxZoom())
    {
        return false;
    }

    const auto timeSupported = isTileTimeSupported();

    {
        QReadLocker scopedLocker(&_lock);

        int res;

        const auto statement = prepareStatement(_database, timeSupported
            ? QStringLiteral("SELECT image, time FROM tiles WHERE x=:x AND y=:y AND z=:z")
            : QStringLiteral("SELECT image FROM tiles WHERE x=:x AND y=:y AND z=:z")
        );
        if (!statement || !configureStatement(statement, tileId, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }
        if ((res = stepStatement(statement)) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to query for %dx%d@%d data: %s",
                tileId.x,
                tileId.y,
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }

        if (res > 0)
        {
            outData = readStatementValue(statement, 0).toByteArray();

            if (timeSupported && pOutTime)
            {
                int64_t time = 0;
                const auto timeValue = readStatementValue(statement, 1);
                if (!timeValue.isNull())
                {
                    bool ok = false;
                    const auto timeLL = timeValue.toLongLong(&ok);
                    if (!ok)
                    {
                        return false;
                    }
                    time = static_cast<int64_t>(timeLL);
                }
                *pOutTime = time;
            }

            return true;
        }
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::storeTileData(
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom,
    const QByteArray& data,
    int64_t time /* = 0 */)
{
    if (!isOpened())
    {
        return false;
    }

    const auto timeSupported = isTileTimeSupported();

    {
        QWriteLocker scopedLocker(&_lock);

        const auto statement = prepareStatement(_database, timeSupported
            ? QStringLiteral("INSERT OR REPLACE INTO tiles(x, y, z, image, time) VALUES(:x, :y, :z, :data, :time)")
            : QStringLiteral("INSERT OR REPLACE INTO tiles(x, y, z, image) VALUES(:x, :y, :z, :data)")
        );
        if (!statement || !configureStatement(statement, tileId, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }
        if (!bindStatementParameter(statement, QStringLiteral(":data"), data))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to bind data for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }
        if (timeSupported && !bindStatementParameter(statement, QStringLiteral(":time"), QVariant(static_cast<qint64>(time))))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to bind time for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }
        
        if (stepStatement(statement) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to store data for %dx%d@%d: %s",
                tileId.x,
                tileId.y,
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }
    }

    if (zoom < getMinZoom() || zoom > getMaxZoom())
    {
        if (!recomputeMinMaxZoom())
        {
            return false;
        }
    }
    if (!recomputeBBox31(zoom))
    {
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::removeTileData(OsmAnd::TileId tileId, OsmAnd::ZoomLevel zoom)
{
    if (!isOpened())
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        const auto statement = prepareStatement(
            _database,
            QStringLiteral("DELETE FROM tiles WHERE x=:x AND y=:y AND z=:z")
        );
        if (!statement || !configureStatement(statement, tileId, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for %dx%d@%d",
                tileId.x,
                tileId.y,
                zoom);
            return false;
        }

        if (stepStatement(statement) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to remove data for %dx%d@%d: %s",
                tileId.x,
                tileId.y,
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }
    }

    if (!recomputeMinMaxZoom())
    {
        return false;
    }
    if (!recomputeBBox31(zoom))
    {
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::removeTilesData()
{
    if (!isOpened())
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        const auto statement = prepareStatement(
            _database,
            QStringLiteral("DELETE FROM tiles")
        );
        if (!statement || stepStatement(statement) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to remove data: %s",
                sqlite3_errmsg(_database.get()));
            return false;
        }
    }

    if (!recomputeMinMaxZoom())
    {
        return false;
    }
    if (!recomputeBBoxes31())
    {
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::removeTilesData(ZoomLevel zoom)
{
    if (!isOpened())
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        const auto statement = prepareStatement(
            _database,
            QStringLiteral("DELETE FROM tiles WHERE z=:z")
        );
        if (!statement || !configureStatement(statement, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for @%d",
                zoom);
            return false;
        }

        if (stepStatement(statement) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to remove data for @%d: %s",
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }
    }

    if (!recomputeMinMaxZoom())
    {
        return false;
    }
    if (!recomputeBBox31(zoom))
    {
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::removeTilesData(AreaI bbox31, bool strict /* = true */ )
{
    if (!isOpened())
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        for (int zoom = ZoomLevel::MinZoomLevel; zoom <= ZoomLevel::MaxZoomLevel; zoom++)
        {
            const auto shift = ZoomLevel31 - zoom;
            const AreaI bbox(
                (bbox31.top() >> shift) + (strict ? 1 : 0),
                (bbox31.left() >> shift) + (strict ? 1 : 0),
                (bbox31.bottom() >> shift) + (strict ? 0 : 1),
                (bbox31.right() >> shift) + (strict ? 0 : 1)
            );

            const auto statement = prepareStatement(
                _database,
                QStringLiteral("DELETE FROM tiles WHERE y>=:t AND x>=:l AND y<=:b AND x<=:r AND z=:z")
            );
            if (!statement || configureStatement(statement, bbox, static_cast<ZoomLevel>(zoom)))
            {
                LogPrintf(
                    LogSeverityLevel::Error,
                    "Failed to configure query for [%d, %d, %d, %d]@%d",
                    bbox.top(), bbox.left(), bbox.bottom(), bbox.right(),
                    zoom);
                return false;
            }
            
            if (stepStatement(statement) < 0)
            {
                LogPrintf(
                    LogSeverityLevel::Error,
                    "Failed to remove data for [%d, %d, %d, %d]@%d: %s",
                    bbox.top(), bbox.left(), bbox.bottom(), bbox.right(),
                    zoom,
                    sqlite3_errmsg(_database.get()));
                return false;
            }
        }
    }

    if (!recomputeMinMaxZoom())
    {
        return false;
    }
    if (!recomputeBBoxes31())
    {
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::removeTilesData(AreaI bbox31, ZoomLevel zoom, bool strict /* = true */)
{
    if (!isOpened())
    {
        return false;
    }

    const auto shift = ZoomLevel31 - zoom;
    const AreaI bbox(
        (bbox31.top() >> shift) + (strict ? 1 : 0),
        (bbox31.left() >> shift) + (strict ? 1 : 0),
        (bbox31.bottom() >> shift) + (strict ? 0 : 1),
        (bbox31.right() >> shift) + (strict ? 0 : 1)
    );

    {
        QWriteLocker scopedLocker(&_lock);

        const auto statement = prepareStatement(
            _database,
            QStringLiteral("DELETE FROM tiles WHERE y>=:t AND x>=:l AND y<=:b AND x<=:r AND z=:z")
        );
        if (!statement || !configureStatement(statement, bbox, zoom))
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to configure query for [%d, %d, %d, %d]@%d",
                bbox.top(), bbox.left(), bbox.bottom(), bbox.right(),
                zoom);
            return false;
        }

        if (stepStatement(statement) < 0)
        {
            LogPrintf(
                LogSeverityLevel::Error,
                "Failed to remove data for [%d, %d, %d, %d]@%d: %s",
                bbox.top(), bbox.left(), bbox.bottom(), bbox.right(),
                zoom,
                sqlite3_errmsg(_database.get()));
            return false;
        }
    }

    if (!recomputeMinMaxZoom())
    {
        return false;
    }
    if (!recomputeBBox31(zoom))
    {
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::compact()
{
    if (!isOpened())
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        if (!vacuum(_database))
        {
            return false;
        }
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::configureStatement(
    const std::shared_ptr<sqlite3_stmt>& statement,
    OsmAnd::AreaI bbox,
    OsmAnd::ZoomLevel zoom) const
{
    auto t = bbox.top();
    auto l = bbox.left();
    auto b = bbox.bottom();
    auto r = bbox.right();

    if (isInvertedY())
    {
        t = (1 << zoom) - 1 - t;
        b = (1 << zoom) - 1 - b;

        qSwap(t, b);
    }

    return configureStatement(statement, zoom)
        && bindStatementParameter(statement, QStringLiteral(":t"), t)
        && bindStatementParameter(statement, QStringLiteral(":l"), l)
        && bindStatementParameter(statement, QStringLiteral(":b"), b)
        && bindStatementParameter(statement, QStringLiteral(":r"), r);
}

bool OsmAnd::TileSqliteDatabase_P::configureStatement(
    const std::shared_ptr<sqlite3_stmt>& statement,
    OsmAnd::TileId tileId,
    OsmAnd::ZoomLevel zoom) const
{
    if (isInvertedY())
    {
        tileId.y = (1 << zoom) - 1 - tileId.y;
    }

    return configureStatement(statement, zoom)
        && bindStatementParameter(statement, QStringLiteral(":x"), tileId.x)
        && bindStatementParameter(statement, QStringLiteral(":y"), tileId.y);
}

bool OsmAnd::TileSqliteDatabase_P::configureStatement(
    const std::shared_ptr<sqlite3_stmt>& statement,
    OsmAnd::ZoomLevel zoom) const
{
    int invertedZoomValue;
    if ((invertedZoomValue = getInvertedZoomValue()) > 0)
    {
        zoom = static_cast<ZoomLevel>(invertedZoomValue - zoom);
    }

    return bindStatementParameter(statement, QStringLiteral(":z"), static_cast<int>(zoom));
}

std::shared_ptr<sqlite3_stmt> OsmAnd::TileSqliteDatabase_P::prepareStatement(
    const std::shared_ptr<sqlite3>& db,
    QString sql)
{
    sqlite3_stmt* pStatement = nullptr;
    const auto res = sqlite3_prepare16_v2(db.get(), sql.utf16(), -1, &pStatement, nullptr);
    const std::shared_ptr<sqlite3_stmt> statement(pStatement, sqlite3_finalize);
    if (res != SQLITE_OK)
    {
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to prepare statement from '%s': %s (%s)",
            qPrintable(sql),
            sqlite3_errmsg(db.get()),
            sqlite3_errstr(res)
        );

        return nullptr;
    }
    return statement;
}

QVariant OsmAnd::TileSqliteDatabase_P::readStatementValue(
    const std::shared_ptr<sqlite3_stmt>& statement,
    int index)
{
    switch (sqlite3_column_type(statement.get(), index))
    {
        case SQLITE_BLOB:
            return QByteArray(
                static_cast<const char *>(sqlite3_column_blob(statement.get(), index)),
                sqlite3_column_bytes(statement.get(), index));
        case SQLITE_INTEGER:
            return sqlite3_column_int64(statement.get(), index);
        case SQLITE_FLOAT:
            return sqlite3_column_double(statement.get(), index);
        case SQLITE_NULL:
            return QVariant();
        default:
            return QString(
                reinterpret_cast<const QChar *>(sqlite3_column_text16(statement.get(), index)),
                sqlite3_column_bytes16(statement.get(), index) / sizeof(QChar));
    }
}

bool OsmAnd::TileSqliteDatabase_P::bindStatementParameter(
    const std::shared_ptr<sqlite3_stmt>& statement,
    QString name,
    QVariant value)
{
    const auto index = sqlite3_bind_parameter_index(statement.get(), name.toUtf8().constData());
    if (index == 0)
    {
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to resolve parameter '%s'",
            qPrintable(name)
        );
        return false;
    }

    return bindStatementParameter(statement, index, value);
}

bool OsmAnd::TileSqliteDatabase_P::bindStatementParameter(
    const std::shared_ptr<sqlite3_stmt>& statement,
    int index,
    QVariant value)
{
    int res;
    switch (value.userType())
    {
        case QVariant::ByteArray:
        {
            const auto data = static_cast<const QByteArray*>(value.constData());
            res = sqlite3_bind_blob(statement.get(), index, data->constData(), data->size(), SQLITE_STATIC);
            break;
        }
        case QVariant::Int:
        case QVariant::Bool:
            res = sqlite3_bind_int(statement.get(), index, value.toInt());
            break;
        case QVariant::Double:
            res = sqlite3_bind_double(statement.get(), index, value.toDouble());
            break;
        case QVariant::UInt:
        case QVariant::LongLong:
            res = sqlite3_bind_int64(statement.get(), index, value.toLongLong());
            break;
        case QVariant::DateTime:
        {
            const auto dateTime = value.toDateTime();
            const auto str = dateTime.toString(Qt::ISODateWithMs);
            res = sqlite3_bind_text16(statement.get(), index, str.utf16(), str.size() * sizeof(QChar), SQLITE_TRANSIENT);
            break;
        }
        case QVariant::Time:
        {
            const auto time = value.toTime();
            const auto str = time.toString(u"hh:mm:ss.zzz");
            res = sqlite3_bind_text16(statement.get(), index, str.utf16(), str.size() * sizeof(QChar), SQLITE_TRANSIENT);
            break;
        }
        case QVariant::String:
        {
            // lifetime of string == lifetime of its qvariant
            const auto pStr = static_cast<const QString*>(value.constData());
            res = sqlite3_bind_text16(statement.get(), index, pStr->utf16(), pStr->size() * sizeof(QChar), SQLITE_STATIC);
            break;
        }
        default:
        {
            const auto str = value.toString();
            // SQLITE_TRANSIENT makes sure that sqlite buffers the data
            res = sqlite3_bind_text16(statement.get(), index, str.utf16(), str.size() * sizeof(QChar), SQLITE_TRANSIENT);
            break;
        }
    }
    if (res != SQLITE_OK)
    {
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to bind parameter #%d value '%s'",
            index,
            qPrintable(value.toString())
        );

        return false;
    }

    return true;
}

int OsmAnd::TileSqliteDatabase_P::stepStatement(const std::shared_ptr<sqlite3_stmt>& statement)
{
    switch(sqlite3_step(statement.get()))
    {
        case SQLITE_ROW:
            return 1;
        case SQLITE_DONE:
            return 0;
        default:
            return -1;
    }
}

bool OsmAnd::TileSqliteDatabase_P::execStatement(const std::shared_ptr<sqlite3>& db, QString sql)
{
    const auto statement = prepareStatement(db, sql);
    return statement && stepStatement(statement) >= 0;
}

std::shared_ptr<OsmAnd::TileSqliteDatabase_P::Meta> OsmAnd::TileSqliteDatabase_P::readMeta(
    const std::shared_ptr<sqlite3>& db)
{
    int res;

    const auto meta = std::make_shared<Meta>();

    const auto selectStatement = prepareStatement(db, QStringLiteral("SELECT * FROM info LIMIT 1"));
    if (!selectStatement)
    {
        return nullptr;
    }

    while ((res = stepStatement(selectStatement)) > 0)
    {
        for (auto fieldIndex = 0, fieldsCount = sqlite3_data_count(selectStatement.get()); fieldIndex < fieldsCount; fieldIndex++)
        {
            const auto fieldName = QString::fromUtf16(
                reinterpret_cast<const ushort*>(sqlite3_column_name16(selectStatement.get(), fieldIndex))
            );
            const auto fieldValue = readStatementValue(selectStatement, fieldIndex);

            meta->values.insert(fieldName, fieldValue);
        }
    }
    if (res < 0)
    {
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to query 'info' table: %s",
            sqlite3_errmsg(db.get()));

        return nullptr;
    }

    return meta;
}

bool OsmAnd::TileSqliteDatabase_P::writeMeta(
    const std::shared_ptr<sqlite3>& db,
    const OsmAnd::TileSqliteDatabase_P::Meta& meta)
{
    if (!execStatement(db, QStringLiteral("DROP TABLE IF EXISTS info")))
    {   
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to drop 'info' table: %s",
            sqlite3_errmsg(db.get()));
        return false;
    }

    auto typedColumns = QStringList();
    for (auto citEntry = meta.values.cbegin(); citEntry != meta.values.cend(); ++citEntry)
    {
        typedColumns.append(QStringLiteral("%1 TEXT").arg(citEntry.key()));
    }

    if (!execStatement(db, QStringLiteral("CREATE TABLE info (%1)").arg(typedColumns.join(QStringLiteral(", ")))))
    {   
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to create 'info' table: %s",
            sqlite3_errmsg(db.get()));
        return false;
    }

    auto columns = QStringList();
    auto values = QStringList();
    auto valuePlaceholders = QStringList();
    for (auto citEntry = meta.values.cbegin(); citEntry != meta.values.cend(); ++citEntry)
    {
        columns.append(citEntry.key());
        values.append(citEntry.value().toString());
        valuePlaceholders.append(QStringLiteral("?"));
    }

    const auto insertStatement = prepareStatement(
        db,
        QStringLiteral("INSERT INTO info (%1) VALUES (%2)")
            .arg(columns.join(QStringLiteral(", ")))
            .arg(valuePlaceholders.join(QStringLiteral(", ")))
    );
    if (!insertStatement)
    {
        return false;
    }
    int bindStatementParameterIndex = 1;
    for (auto citValue = values.cbegin(); citValue != values.cend(); ++citValue, bindStatementParameterIndex++)
    {
        bindStatementParameter(insertStatement, bindStatementParameterIndex, *citValue);
    }
    if (stepStatement(insertStatement) < 0)
    {   
        LogPrintf(
            LogSeverityLevel::Error,
            "Failed to insert into 'info' table: %s",
            sqlite3_errmsg(db.get()));
        return false;
    }

    return true;
}

bool OsmAnd::TileSqliteDatabase_P::vacuum(const std::shared_ptr<sqlite3>& db)
{
    return execStatement(db, QStringLiteral("VACUUM"));
}
