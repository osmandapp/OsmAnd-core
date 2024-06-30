#ifndef _OSMAND_CORE_TILE_SQLITE_DATABASE_H_
#define _OSMAND_CORE_TILE_SQLITE_DATABASE_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QVariant>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class TileSqliteDatabase_P;
    class OSMAND_CORE_API TileSqliteDatabase
    {
        Q_DISABLE_COPY_AND_MOVE(TileSqliteDatabase);
    public:
        struct OSMAND_CORE_API Meta Q_DECL_FINAL
        {
            Meta();
            ~Meta();

            QHash<QString, QVariant> values;

            static const QString TITLE;
            QString getTitle(bool* outOk = nullptr) const;
            void setTitle(QString title);

            static const QString RULE;
            QString getRule(bool* outOk = nullptr) const;
            void setRule(QString rule);

            static const QString REFERER;
            QString getReferer(bool* outOk = nullptr) const;
            void setReferer(QString referer);
            
            static const QString USER_AGENT;
            QString getUserAgent(bool* outOk = nullptr) const;
            void setUserAgent(QString userAgent);

            static const QString RANDOMS;
            QString getRandoms(bool* outOk = nullptr) const;
            void setRandoms(QString randoms);

            static const QString URL;
            QString getUrl(bool* outOk = nullptr) const;
            void setUrl(QString url);

            static const QString MIN_ZOOM;
            int64_t getMinZoom(bool* outOk = nullptr) const;
            void setMinZoom(int64_t minZoom);

            static const QString MAX_ZOOM;
            int64_t getMaxZoom(bool* outOk = nullptr) const;
            void setMaxZoom(int64_t maxZoom);

            static const QString ELLIPSOID;
            int64_t getEllipsoid(bool* outOk = nullptr) const;
            void setEllipsoid(int64_t ellipsoid);
            
            static const QString INVERTED_Y;
            int64_t getInvertedY(bool* outOk = nullptr) const;
            void setInvertedY(int64_t invertedY);

            static const QString TIME_COLUMN;
            QString getTimeColumn(bool* outOk = nullptr) const;
            void setTimeColumn(QString timeColumn);

            static const QString TIMESTAMP;
            QString getTimestamp(bool* outOk = nullptr) const;
            void setTimestamp(QString timestamp);

            static const QString SPECIFICATED;
            QString getSpecificated(bool* outOk = nullptr) const;
            void setSpecificated(QString specificated);

            static const QString VALUE_RANGE;
            QString getValueRange(bool* outOk = nullptr) const;
            void setValueRange(QString valueRange);

            static const QString EXPIRE_MINUTES;
            int64_t getExpireMinutes(bool* outOk = nullptr) const;
            void setExpireMinutes(int64_t expireMinutes);

            static const QString TILE_NUMBERING;
            QString getTileNumbering(bool* outOk = nullptr) const;
            void setTileNumbering(QString tileNumbering);

            static const QString TILE_SIZE;
            int64_t getTileSize(bool* outOk = nullptr) const;
            void setTileSize(int64_t tileSize);
        };
    private:
        PrivateImplementation<TileSqliteDatabase_P> _p;
    protected:
    public:
        TileSqliteDatabase();
        TileSqliteDatabase(QString filename);
        virtual ~TileSqliteDatabase();

        const QString filename;

        bool isOpened() const;
        bool open(const bool withSpecification = false);
        bool close(bool compact = true);
        bool isOnlineTileSource() const;

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

        bool compact();
    };

}

#endif // !defined(_OSMAND_CORE_TILE_SQLITE_DATABASE_H_)
