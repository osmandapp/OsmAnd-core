#ifndef _OSMAND_CORE_TILE_SQLITE_DATABASE_P_H_
#define _OSMAND_CORE_TILE_SQLITE_DATABASE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QAtomicInt>
#include <QReadWriteLock>
#include <QMutex>
#include <QVariant>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <sqlite3.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "TileSqliteDatabase.h"

namespace OsmAnd
{
    class TileSqliteDatabase;

    class TileSqliteDatabase_P Q_DECL_FINAL
    {
    public:
        typedef TileSqliteDatabase::Meta Meta;

    private:
        mutable QReadWriteLock _lock;
        std::shared_ptr<sqlite3> _database;
        QAtomicInt _isOpened;

        mutable QMutex _metaLock;
        mutable std::shared_ptr<Meta> _meta;

        mutable QAtomicInteger<int32_t> _cachedMinZoom;
        mutable QAtomicInteger<int32_t> _cachedMaxZoom;
        mutable QAtomicInt _cachedInvertedZoom;
        mutable QAtomicInt _cachedInvertedY;
        mutable QAtomicInt _cachedIsTileTimeSupported;
        mutable QAtomicInt _cachedIsTileTimestampSupported;
        mutable QAtomicInt _cachedIsTileSpecificationSupported;
        mutable QAtomicInt _cachedIsTileValueRangeSupported;
        mutable QReadWriteLock _cachedBboxes31Lock;
        mutable AreaI _cachedBbox31;
        mutable std::array<AreaI, ZoomLevelsCount> _cachedBboxes31;
        void resetCachedInfo();
        bool setMinMaxZoom(OsmAnd::ZoomLevel minZoom, OsmAnd::ZoomLevel maxZoom);

    protected:
        TileSqliteDatabase_P(TileSqliteDatabase* owner);

        bool configureStatement(bool invertedY, int invertedZoomValue, const std::shared_ptr<sqlite3_stmt>& statement,
            AreaI bbox, ZoomLevel zoom) const;
        bool configureStatement(bool invertedY, int invertedZoomValue, const std::shared_ptr<sqlite3_stmt>& statement,
            TileId tileId, ZoomLevel zoom, const int64_t specification = 0) const;
        bool configureStatement(bool invertedY, int invertedZoomValue, const std::shared_ptr<sqlite3_stmt>& statement,
            QList<TileId>& tileIds, ZoomLevel zoom, const int64_t specification = 0) const;
        bool configureStatement(int invertedZoomValue, const std::shared_ptr<sqlite3_stmt>& statement,
            ZoomLevel zoom) const;
        bool configureStatementSpecification(const std::shared_ptr<sqlite3_stmt>& statement,
            int64_t specification) const;
        bool configureStatement(const std::shared_ptr<sqlite3_stmt>& statement, int64_t time) const;

        static std::shared_ptr<sqlite3_stmt> prepareStatement(const std::shared_ptr<sqlite3>& db, QString sql);
        static QVariant readStatementValue(const std::shared_ptr<sqlite3_stmt>& statement,
            int index, void* data = nullptr);
        static bool bindStatementParameter(const std::shared_ptr<sqlite3_stmt>& statement,
            QString name, QVariant value);
        static bool bindStatementParameter(const std::shared_ptr<sqlite3_stmt>& statement, int index, QVariant value);
        static int stepStatement(const std::shared_ptr<sqlite3_stmt>& statement);
        static bool execStatement(const std::shared_ptr<sqlite3>& db, QString sql);

        static std::shared_ptr<Meta> readMeta(const std::shared_ptr<sqlite3>& db);
        static bool writeMeta(const std::shared_ptr<sqlite3>& db, const Meta& meta);

        static bool vacuum(const std::shared_ptr<sqlite3>& db);

        int getInvertedZoomValue() const;
        bool isInvertedY() const;
    public:
        ~TileSqliteDatabase_P();

        ImplementationInterface<TileSqliteDatabase> owner;

        bool isOpened() const;
        bool open(const bool withSpecification = false);
        bool close(bool compact = true);

        bool isTileTimeSupported() const;
        bool hasTimeColumn() const;
        bool enableTileTimeSupport(bool force = false);

        bool isTileTimestampSupported() const;
        bool hasTimestampColumn() const;
        bool enableTileTimestampSupport(bool force = false);

        bool isTileSpecificationSupported() const;
        bool hasSpecificationColumn() const;

        bool isTileValueRangeSupported() const;
        bool hasValueRangeColumn() const;
        bool enableValueRangeSupport(bool force = false);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;
        
        bool recomputeMinMaxZoom();

        AreaI getBBox31() const;
        bool recomputeBBox31(AreaI* pOutBBox31 = nullptr);

        AreaI getBBox31(ZoomLevel zoom) const;
        bool recomputeBBox31(ZoomLevel zoom, AreaI* pOutBBox31 = nullptr);

        std::array<AreaI, ZoomLevelsCount> getBBoxes31() const;
        bool recomputeBBoxes31(
            std::array<AreaI, ZoomLevelsCount>* pOutBBoxes31 = nullptr,
            AreaI* pOutBBox31 = nullptr
        );

        bool obtainMeta(Meta& outMeta) const;
        bool storeMeta(const Meta& meta);

        bool isEmpty() const;
        bool getTileIds(QList<TileId>& tileIds, ZoomLevel zoom, int64_t specification = 0);
        bool getTilesSize(QList<TileId> tileIds, uint64_t& size, ZoomLevel zoom, int64_t specification = 0);
        bool containsTileData(TileId tileId, ZoomLevel zoom, int64_t specification = 0) const;
        bool obtainTileTime(TileId tileId, ZoomLevel zoom, int64_t& outTime, int64_t specification = 0) const;
        bool retrieveTileData(TileId tileId, ZoomLevel zoom, QByteArray& outData, int64_t* pOutTime = nullptr) const;
        bool retrieveTileData(TileId tileId, ZoomLevel zoom, int64_t specification,
            QByteArray& outData, int64_t* pOutTime = nullptr, int64_t* pOutTimestamp = nullptr) const;
        bool obtainTileData(TileId tileId, ZoomLevel zoom, void* outData, int64_t* pOutTime = nullptr) const;
        bool obtainTileData(TileId tileId, ZoomLevel zoom, void* outData, float& minValue, float& maxValue) const;
        bool obtainTileData(TileId tileId, ZoomLevel zoom, int64_t specification,
            void* outData, int64_t* pOutTime = nullptr) const;
        bool storeTileData(TileId tileId, ZoomLevel zoom, const QByteArray& data, int64_t time = 0);
        bool storeTileData(TileId tileId, ZoomLevel zoom, int64_t specification, const QByteArray& data,
            int64_t time = 0, int64_t timestamp = 0, float minValue = 0, float maxValue = 0);
        bool updateTileTimestamp(TileId tileId, ZoomLevel zoom, int64_t timestamp);
        bool updateTileDataFrom(const QString& dbFilePath, const QString* specName = nullptr);
        bool removeTileData(TileId tileId, ZoomLevel zoom, int64_t specification = 0);
        bool removeTilesData(QList<TileId>& tileIds, ZoomLevel zoom, int64_t specification = 0);
        bool removeTilesData();
        bool removeTilesData(ZoomLevel zoom);
        bool removeBiggerTilesData(ZoomLevel zoom);
        bool removeSpecificTilesData(int64_t specification);
        bool removePreviousTilesData(int64_t specification);
        bool removeOlderTilesData(int64_t time);
        bool removeTilesData(AreaI bbox31, bool strict = true);
        bool removeTilesData(AreaI bbox31, ZoomLevel zoom, bool strict = true);
        
        bool isOnlineTileSource() const;
        bool compact();

        friend class OsmAnd::TileSqliteDatabase;
    };
}

#endif // !defined(_OSMAND_CORE_TILE_SQLITE_DATABASE_P_H_)
