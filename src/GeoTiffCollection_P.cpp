#include "GeoTiffCollection_P.h"
#include "GeoTiffCollection.h"

#include <cassert>

#include <gdal_priv.h>
#include <cpl_conv.h>

#include "QtCommon.h"

#include "OsmAndCore_private.h"
#include "QKeyValueIterator.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::GeoTiffCollection_P::GeoTiffCollection_P(
    GeoTiffCollection* owner_,
    bool useFileWatcher /*= true*/)
    : _fileSystemWatcher(useFileWatcher ? new QFileSystemWatcher() : NULL)
    , _lastUnusedSourceOriginId(0)
    , _collectedSourcesInvalidated(1)
    , owner(owner_)
{
    _minZoom.storeRelease(ZoomLevel9);
    _pixelSize31.storeRelease(0);
    if (_fileSystemWatcher)
    {
        _fileSystemWatcher->moveToThread(gMainThread);

        _onDirectoryChangedConnection = QObject::connect(
            _fileSystemWatcher, &QFileSystemWatcher::directoryChanged,
            (std::function<void(const QString&)>)std::bind(&GeoTiffCollection_P::onDirectoryChanged, this, std::placeholders::_1));
        _onFileChangedConnection = QObject::connect(
            _fileSystemWatcher, &QFileSystemWatcher::fileChanged,
            (std::function<void(const QString&)>)std::bind(&GeoTiffCollection_P::onFileChanged, this, std::placeholders::_1));
    }
    earthInMeters = 2.0 * M_PI * 6378137;
    earthIn31 = 1u << MaxZoomLevel;
}

OsmAnd::GeoTiffCollection_P::~GeoTiffCollection_P()
{
    if (_fileSystemWatcher)
    {
        QObject::disconnect(_onDirectoryChangedConnection);
        QObject::disconnect(_onFileChangedConnection);

        _fileSystemWatcher->deleteLater();
    }
}

void OsmAnd::GeoTiffCollection_P::invalidateCollectedSources()
{
    _pixelSize31.storeRelease(0);
    _collectedSourcesInvalidated.fetchAndAddOrdered(1);
}

void OsmAnd::GeoTiffCollection_P::clearCollectedSources() const
{
    QWriteLocker scopedLocker(&_collectedSourcesLock);

    // Check all previously collected sources
    auto itCollectedSourcesEntry = mutableIteratorOf(_collectedSources);
    while(itCollectedSourcesEntry.hasNext())
        itCollectedSourcesEntry.remove();
}

void OsmAnd::GeoTiffCollection_P::collectSources() const
{
    QWriteLocker scopedLocker1(&_collectedSourcesLock);
    QReadLocker scopedLocker2(&_sourcesOriginsLock);

    // Capture how many invalidations are going to be processed
    const auto invalidationsToProcess = _collectedSourcesInvalidated.loadAcquire();
    if (invalidationsToProcess == 0)
        return;

    const Stopwatch collectSourcesStopwatch(true);

    // Check all previously collected sources
    auto itCollectedSourcesEntry = mutableIteratorOf(_collectedSources);
    while(itCollectedSourcesEntry.hasNext())
    {
        const auto& collectedSourcesEntry = itCollectedSourcesEntry.next();
        const auto& originId = collectedSourcesEntry.key();
        auto& collectedSources = collectedSourcesEntry.value();
        const auto& itSourceOrigin = _sourcesOrigins.constFind(originId);

        // If current source origin was removed,
        // remove entire each collected source related to it
        if (itSourceOrigin == _sourcesOrigins.cend())
        {
            // Ensure that file is not being read anywhere
            for(const auto& itCollectedSource : rangeOf(collectedSources))
            {
                if (auto& database = itCollectedSource.value().cacheDatabase)
                {
                    database->close();
                    database.reset();
                }
            }

            itCollectedSourcesEntry.remove();
            continue;
        }

        // Check for missing files
        auto itFileEntry = mutableIteratorOf(collectedSources);
        while(itFileEntry.hasNext())
        {
            const auto& fileEntry = itFileEntry.next();
            const auto& sourceFilePath = fileEntry.key();
            if (QFile::exists(sourceFilePath))
                continue;            
            if (auto& database = fileEntry.value().cacheDatabase)
            {
                database->close();
                database.reset();
            }
            itFileEntry.remove();
        }

        // If all collected sources for current source origin are gone,
        // remove entire collection attached to source origin ID
        if (collectedSources.isEmpty())
        {
            itCollectedSourcesEntry.remove();
            continue;
        }
    }

    // Find all files uncollected sources
    for(const auto& itEntry : rangeOf(constOf(_sourcesOrigins)))
    {
        const auto& originId = itEntry.key();
        const auto& entry = itEntry.value();
        auto itCollectedSources = _collectedSources.find(originId);
        if (itCollectedSources == _collectedSources.end())
            itCollectedSources = _collectedSources.insert(originId, QHash<QString, GeoTiffProperties>());
        auto& collectedSources = *itCollectedSources;

        if (entry->type == SourceOriginType::Directory)
        {
            const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(entry);

            QFileInfoList filesInfo;
            Utilities::findFiles(
                directoryAsSourceOrigin->directory,
                QStringList() << QStringLiteral("*.tif"),
                filesInfo,
                directoryAsSourceOrigin->isRecursive
            );
            for(const auto& fileInfo : constOf(filesInfo))
            {
                const auto& filePath = fileInfo.canonicalFilePath();
                const auto citFile = collectedSources.constFind(filePath);
                if (citFile == collectedSources.cend())
                    collectedSources.insert(filePath, getGeoTiffProperties(filePath));
            }

            if (_fileSystemWatcher && directoryAsSourceOrigin->isRecursive)
            {
                QFileInfoList directoriesInfo;
                Utilities::findDirectories(
                    directoryAsSourceOrigin->directory,
                    QStringList() << QStringLiteral("*"),
                    directoriesInfo,
                    true
                );

                for(const auto& directoryInfo : constOf(directoriesInfo))
                {
                    const auto canonicalPath = directoryInfo.canonicalFilePath();
                    if (directoryAsSourceOrigin->watchedSubdirectories.contains(canonicalPath))
                        continue;

                    _fileSystemWatcher->addPath(canonicalPath);
                    directoryAsSourceOrigin->watchedSubdirectories.insert(canonicalPath);
                }
            }
        }
        else if (entry->type == SourceOriginType::File)
        {
            const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(entry);

            if (!fileAsSourceOrigin->fileInfo.exists())
                continue;
            const auto& filePath = fileAsSourceOrigin->fileInfo.canonicalFilePath();
            const auto citFile = collectedSources.constFind(filePath);
            if (citFile != collectedSources.cend())
                continue;
            collectedSources.insert(filePath, getGeoTiffProperties(filePath));
        }
    }

    // Decrement invalidations counter with number of processed onces
    _collectedSourcesInvalidated.fetchAndAddOrdered(-invalidationsToProcess);

    LogPrintf(LogSeverityLevel::Info, "Collected tile sources in %fs", collectSourcesStopwatch.elapsed());
}

QList<OsmAnd::GeoTiffCollection::SourceOriginId> OsmAnd::GeoTiffCollection_P::getSourceOriginIds() const
{
    QReadLocker scopedLocker(&_sourcesOriginsLock);

    return _sourcesOrigins.keys();
}

inline int32_t OsmAnd::GeoTiffCollection_P::metersTo31(const double positionInMeters) const
{
    auto positionIn31 = static_cast<int64_t>(std::floor((positionInMeters / earthInMeters + 0.5) * earthIn31));
    if (positionIn31 > INT32_MAX)
        positionIn31 = positionIn31 - INT32_MAX - 1;
    else if (positionIn31 < 0)
        positionIn31 = positionIn31 + INT32_MAX + 1;
    return static_cast<int32_t>(positionIn31);
}

inline OsmAnd::PointI OsmAnd::GeoTiffCollection_P::metersTo31(const PointD& locationInMeters) const
{
    return PointI(metersTo31(locationInMeters.x), INT32_MAX - metersTo31(locationInMeters.y));
}

inline double OsmAnd::GeoTiffCollection_P::metersFrom31(const double position31) const
{
    return (position31 / earthIn31 - 0.5) * earthInMeters;
}

inline OsmAnd::PointD OsmAnd::GeoTiffCollection_P::metersFrom31(const double positionX, const double positionY) const
{
    return PointD(metersFrom31(positionX), metersFrom31(earthIn31 - positionY));
}

inline bool OsmAnd::GeoTiffCollection_P::containsTile(
    const AreaI& region31, const AreaI& area31) const
{
    if (region31.contains(area31))
        return true;
    if (area31.left() >= 0 && region31.contains(AreaI(
        area31.top(),
        area31.left() - INT32_MAX - 1,
        area31.bottom(),
        area31.right() - INT32_MAX - 1)))
        return true;
    if (area31.top() >= 0 && region31.contains(AreaI(
        area31.top() - INT32_MAX - 1,
        area31.left(),
        area31.bottom() - INT32_MAX - 1,
        area31.right())))
        return true;
    if (area31.left() >= 0 && area31.top() >= 0 && region31.contains(AreaI(
        area31.top() - INT32_MAX - 1,
        area31.left() - INT32_MAX - 1,
        area31.bottom() - INT32_MAX - 1,
        area31.right() - INT32_MAX - 1)))
        return true;
    return false;
}

inline OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::calcMaxZoom(
    const int32_t pixelSize31, const uint32_t tileSize) const
{
    const auto tileSize31 = qBound(1.0, static_cast<double>(pixelSize31) * tileSize, earthIn31);
    return static_cast<ZoomLevel>(MaxZoomLevel - std::ceil(log2(tileSize31)));
}

inline OsmAnd::GeoTiffCollection_P::GeoTiffProperties OsmAnd::GeoTiffCollection_P::getGeoTiffProperties(
    const QString& filePath) const
{   
    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filePath), GA_ReadOnly))
    {
        const auto rasterBandCount = dataset->GetRasterCount();
        auto bandDataType = GDT_Unknown;
        GDALRasterBand* band;
        bool result = rasterBandCount > 0 && rasterBandCount < 10 && (band = dataset->GetRasterBand(1));
        result = result && (bandDataType = band->GetRasterDataType()) != GDT_Unknown;
        result = result && !band->GetColorTable();
        double geoTransform[6];
        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
        {
            QMutexLocker scopedLocker(&_localCacheDirMutex);

            QFileInfo sourceFile(filePath);
            const auto dbFilePath = (_localCacheDir.path().isEmpty() ? filePath :
                _localCacheDir.path() + QDir::separator() + sourceFile.fileName()) + QStringLiteral(".sqlite");
            const auto cacheDb = std::make_shared<TileSqliteDatabase>(dbFilePath);
            if (cacheDb->open(true))
            {
                GDALClose(dataset);
                auto upperLeft31 = metersTo31(PointD(geoTransform[0], geoTransform[3]));
                auto lowerRight31 = metersTo31(PointD(
                    geoTransform[0] + geoTransform[1] * dataset->GetRasterXSize(),
                    geoTransform[3] + geoTransform[5] * dataset->GetRasterYSize()));
                if (upperLeft31.x > lowerRight31.x)
                    upperLeft31.x = upperLeft31.x - INT32_MAX - 1;
                if (upperLeft31.y > lowerRight31.y)
                    upperLeft31.y = upperLeft31.y - INT32_MAX - 1;
                const int32_t pixelSize31 =
                    std::ceil(qBound(0.0, geoTransform[1] * earthIn31 / earthInMeters, earthIn31 - 1.0));
                TileSqliteDatabase::Meta meta;
                if (!cacheDb->obtainMeta(meta))
                {
                    meta.setMinZoom(getMinZoom());
                    meta.setMaxZoom(MaxZoomLevel);
                    meta.setTileNumbering(QStringLiteral(""));
                    meta.setSpecificated(QStringLiteral("yes"));
                    cacheDb->storeMeta(meta);
                }
                return GeoTiffProperties(upperLeft31, lowerRight31, pixelSize31, rasterBandCount, bandDataType, cacheDb);
            }
        }
        GDALClose(dataset);
    }
    return GeoTiffProperties();
}

OsmAnd::GeoTiffCollection::SourceOriginId OsmAnd::GeoTiffCollection_P::addDirectory(
    const QDir& dir,
    bool recursive)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto allocatedId = _lastUnusedSourceOriginId++;
    auto sourceOrigin = new DirectoryAsSourceOrigin();
    sourceOrigin->directory = dir;
    sourceOrigin->isRecursive = recursive;
    _sourcesOrigins.insert(allocatedId, qMove(std::shared_ptr<const SourceOrigin>(sourceOrigin)));

    if (_fileSystemWatcher)
    {
        _fileSystemWatcher->addPath(dir.canonicalPath());
        if (recursive)
        {
            QFileInfoList subdirs;
            Utilities::findDirectories(dir, QStringList() << QStringLiteral("*"), subdirs, true);
            for(const auto& subdir : subdirs)
            {
                const auto canonicalPath = subdir.canonicalFilePath();
                sourceOrigin->watchedSubdirectories.insert(canonicalPath);
                _fileSystemWatcher->addPath(canonicalPath);
            }
        }
    }

    invalidateCollectedSources();

    return allocatedId;
}

OsmAnd::GeoTiffCollection::SourceOriginId OsmAnd::GeoTiffCollection_P::addFile(
    const QFileInfo& fileInfo)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto allocatedId = _lastUnusedSourceOriginId++;
    auto sourceOrigin = new GeoTiffCollection_P::FileAsSourceOrigin();
    sourceOrigin->fileInfo = fileInfo;
    _sourcesOrigins.insert(allocatedId, qMove(std::shared_ptr<const SourceOrigin>(sourceOrigin)));

    if (_fileSystemWatcher)
        _fileSystemWatcher->addPath(fileInfo.canonicalFilePath());

    invalidateCollectedSources();

    return allocatedId;
}

bool OsmAnd::GeoTiffCollection_P::removeFile(const QFileInfo& fileInfo)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    for(const auto& itEntry : rangeOf(constOf(_sourcesOrigins)))
    {
        const auto& originId = itEntry.key();
        const auto& entry = itEntry.value();
        
        if (entry->type == SourceOriginType::File)
        {
            const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(entry);
            if (fileAsSourceOrigin->fileInfo == fileInfo)
            {
                if (_fileSystemWatcher)
                    _fileSystemWatcher->removePath(fileAsSourceOrigin->fileInfo.canonicalFilePath());
                
                _sourcesOrigins.remove(originId);

                invalidateCollectedSources();
                return true;
            }
        }
    }
    return false;
}

bool OsmAnd::GeoTiffCollection_P::remove(const GeoTiffCollection::SourceOriginId entryId)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto itSourceOrigin = _sourcesOrigins.find(entryId);
    if (itSourceOrigin == _sourcesOrigins.end())
        return false;

    if (_fileSystemWatcher)
    {
        const auto& sourceOrigin = *itSourceOrigin;
        if (sourceOrigin->type == SourceOriginType::Directory)
        {
            const auto& directoryAsSourceOrigin =
                std::static_pointer_cast<const DirectoryAsSourceOrigin>(sourceOrigin);

            for(const auto& watchedSubdirectory : constOf(directoryAsSourceOrigin->watchedSubdirectories))
                _fileSystemWatcher->removePath(watchedSubdirectory);
            _fileSystemWatcher->removePath(directoryAsSourceOrigin->directory.canonicalPath());
        }
        else if (sourceOrigin->type == SourceOriginType::File)
        {
            const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(sourceOrigin);
            _fileSystemWatcher->removePath(fileAsSourceOrigin->fileInfo.canonicalFilePath());
        }
    }
    
    _sourcesOrigins.erase(itSourceOrigin);

    invalidateCollectedSources();

    return true;
}

void OsmAnd::GeoTiffCollection_P::setLocalCache(const QDir& localCacheDir)
{
    {
        QWriteLocker scopedLocker1(&_collectedSourcesLock);
        QMutexLocker scopedLocker2(&_localCacheDirMutex);
        _localCacheDir = localCacheDir;
    }

    clearCollectedSources();

    invalidateCollectedSources();
}

void OsmAnd::GeoTiffCollection_P::setMinZoom(ZoomLevel zoomLevel) const
{
    _minZoom.storeRelease(zoomLevel);
}

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::getMinZoom() const
{
    return static_cast<ZoomLevel>(_minZoom.loadAcquire());
}

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::getMaxZoom(const uint32_t tileSize) const
{
    int32_t pixelSize31;
    if ((pixelSize31 = _pixelSize31.loadAcquire()) > 0)
    {
        return calcMaxZoom(pixelSize31, tileSize);
    }

    auto maxZoom = InvalidZoomLevel;
    QReadLocker scopedLocker(&_collectedSourcesLock);
    
    for (const auto& collectedSources : constOf(_collectedSources))
    {
        for (const auto& fileProperties : constOf(collectedSources))
        {
            if (fileProperties.pixelSize31 > pixelSize31)
            {
                pixelSize31 = fileProperties.pixelSize31;
                maxZoom = calcMaxZoom(pixelSize31, tileSize);
            }
        }
    }
    if (pixelSize31 > 0)
        _pixelSize31.storeRelease(pixelSize31);

    return maxZoom;
}

bool OsmAnd::GeoTiffCollection_P::getGeoTiffData(const TileId& tileId, const ZoomLevel zoom, const uint32_t tileSize,
    const uint32_t overlap, const uint32_t bandCount, const bool toBytes, void *pBuffer) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();
    
    // Calculate tile edges to find corresponding data
    const auto minZoom = getMinZoom();
    const auto maxZoom = getMaxZoom(tileSize - overlap);
    if (zoom < minZoom || zoom > maxZoom)
        return false;
    const auto zoomDelta = MaxZoomLevel - zoom;
    const auto overlapOffset = static_cast<double>(1u << zoomDelta) * 0.5 *
        static_cast<double>(overlap) / static_cast<double>(tileSize - overlap);
    PointI upperLeft31(tileId.x << zoomDelta, tileId.y << zoomDelta);
    PointI lowerRight31;
    PointI64 lowerRight64(tileId.x, tileId.y);
    lowerRight64.x = (lowerRight64.x + 1) << zoomDelta;
    lowerRight64.y = (lowerRight64.y + 1) << zoomDelta;
    PointD upperLeftNative(
        static_cast<double>(upperLeft31.x) - overlapOffset,
        static_cast<double>(upperLeft31.y) - overlapOffset);
    PointD lowerRightNative(
        static_cast<double>(lowerRight64.x) + overlapOffset,
        static_cast<double>(lowerRight64.y) + overlapOffset);
    upperLeft31.x = std::floor(upperLeftNative.x);
    upperLeft31.y = std::floor(upperLeftNative.y);
    lowerRight64.x = std::floor(lowerRightNative.x);
    lowerRight64.y = std::floor(lowerRightNative.y);
    if (lowerRight64.x > INT32_MAX)
    {
        upperLeft31.x = upperLeft31.x - INT32_MAX - 1;
        lowerRight64.x = lowerRight64.x - INT32_MAX - 1;
    }
    if (lowerRight64.y > INT32_MAX)
    {
        upperLeft31.y = upperLeft31.y - INT32_MAX - 1;
        lowerRight64.y = lowerRight64.y - INT32_MAX - 1;
    }
    lowerRight31.x = lowerRight64.x;
    lowerRight31.y = lowerRight64.y;

    PointD upperLeftOverscaled;
    PointD lowerRightOverscaled;

    GDALRasterIOExtraArg extraArg;
    extraArg.nVersion = RASTERIO_EXTRA_ARG_CURRENT_VERSION;
    extraArg.eResampleAlg = GRIORA_Average;
    extraArg.pfnProgress = CPL_NULLPTR;
    extraArg.pProgressData = CPL_NULLPTR;
    extraArg.bFloatingPointWindowValidity = TRUE;

    // Calculate data properties
    const auto specification = tileSize * 1000 + overlap * 100 + bandCount * 10 + (toBytes ? 1 : 4);
    const auto destDataType = toBytes ? GDT_Byte : GDT_Float32;
    const auto pixelSizeInBytes = destDataType == GDT_Byte ? 1 : 4;
    const auto bufferSize = bandCount * tileSize * tileSize * pixelSizeInBytes;

    QReadLocker scopedLocker(&_collectedSourcesLock);

    for (const auto& collectedSources : constOf(_collectedSources))
    {
        for (const auto& itEntry : rangeOf(constOf(collectedSources)))
        {
            const auto& filePath = itEntry.key();
            const auto& tiffProperties = itEntry.value();
            if (bandCount <= tiffProperties.rasterBandCount &&
                containsTile(tiffProperties.region31, AreaI(upperLeft31, lowerRight31)))
            {
                const int zoomShift = zoom - minZoom;
                bool tileFound = true;
                if (zoomShift > 0 && minZoom > MinZoomLevel)
                {
                    // Make sure if tile can be represented by any overscaled one
                    const auto zoomDeltaOverscaled = MaxZoomLevel - minZoom;
                    const auto overlapOffsetOverscaled = static_cast<double>(1u << zoomDeltaOverscaled) * 0.5 *
                        static_cast<double>(overlap) / static_cast<double>(tileSize - overlap);
                    PointI lowerRight31Overscaled(tileId.x >> zoomShift, tileId.y >> zoomShift);
                    lowerRight64.x = lowerRight31Overscaled.x;
                    lowerRight64.y = lowerRight31Overscaled.y;
                    lowerRight64.x = (lowerRight64.x + 1) << zoomDeltaOverscaled;
                    lowerRight64.y = (lowerRight64.y + 1) << zoomDeltaOverscaled;
                    PointI upperLeft31Overscaled(
                        lowerRight31Overscaled.x << zoomDeltaOverscaled,
                        lowerRight31Overscaled.y << zoomDeltaOverscaled);
                    upperLeftOverscaled.x = static_cast<double>(upperLeft31Overscaled.x) - overlapOffsetOverscaled;
                    upperLeftOverscaled.y = static_cast<double>(upperLeft31Overscaled.y) - overlapOffsetOverscaled;
                    lowerRightOverscaled.x = static_cast<double>(lowerRight64.x) + overlapOffsetOverscaled;
                    lowerRightOverscaled.y = static_cast<double>(lowerRight64.y) + overlapOffsetOverscaled;
                    upperLeft31Overscaled.x = std::floor(upperLeftOverscaled.x);
                    upperLeft31Overscaled.y = std::floor(upperLeftOverscaled.y);
                    lowerRight64.x = std::floor(lowerRightOverscaled.x);
                    lowerRight64.y = std::floor(lowerRightOverscaled.y);
                    if (lowerRight64.x > INT32_MAX)
                    {
                        upperLeft31Overscaled.x = upperLeft31Overscaled.x - INT32_MAX - 1;
                        lowerRight64.x = lowerRight64.x - INT32_MAX - 1;
                    }
                    if (lowerRight64.y > INT32_MAX)
                    {
                        upperLeft31Overscaled.y = upperLeft31Overscaled.y - INT32_MAX - 1;
                        lowerRight64.y = lowerRight64.y - INT32_MAX - 1;
                    }
                    lowerRight31Overscaled.x = lowerRight64.x;
                    lowerRight31Overscaled.y = lowerRight64.y;
                    if (!containsTile(tiffProperties.region31, AreaI(upperLeft31Overscaled, lowerRight31Overscaled)))
                        tileFound = false;
                }

                if (tileFound)
                {
                    std::shared_ptr<OsmAnd::TileSqliteDatabase> cacheDatabase;
                        
                    // Check if data is available in cache file
                    for (const auto& collectedSources : constOf(_collectedSources))
                    {
                        const auto tiffProperties = collectedSources.constFind(filePath);
                        if (tiffProperties != collectedSources.cend())
                        {
                            cacheDatabase = tiffProperties->cacheDatabase;
                            if (cacheDatabase->isOpened() &&
                                cacheDatabase->obtainTileData(tileId, zoom, specification, pBuffer))
                                return true;
                        }
                    }

                    bool result = false;
                    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filePath), GA_ReadOnly))
                    {
                        // Read raster data from source Geotiff file
                        const auto rasterBandCount = dataset->GetRasterCount();
                        GDALRasterBand* band;
                        result = rasterBandCount > 0 && rasterBandCount < 10 && rasterBandCount >= bandCount;
                        double geoTransform[6];
                        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
                            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
                        {
                            auto upperLeft = metersFrom31(upperLeftNative.x, upperLeftNative.y);
                            auto lowerRight = metersFrom31(lowerRightNative.x, lowerRightNative.y);
                            upperLeft.x = (upperLeft.x - geoTransform[0]) / geoTransform[1];
                            upperLeft.y = (upperLeft.y - geoTransform[3]) / geoTransform[5];
                            lowerRight.x = (lowerRight.x - geoTransform[0]) / geoTransform[1];
                            lowerRight.y = (lowerRight.y - geoTransform[3]) / geoTransform[5];
                            PointI dataOffset(std::floor(upperLeft.x), std::floor(upperLeft.y));
                            PointI dataSize(
                                static_cast<int32_t>(std::ceil(lowerRight.x)) - dataOffset.x,
                                static_cast<int32_t>(std::ceil(lowerRight.y)) - dataOffset.y);
                            extraArg.dfXOff = upperLeft.x;
                            extraArg.dfYOff = upperLeft.y;
                            extraArg.dfXSize = lowerRight.x - upperLeft.x;
                            extraArg.dfYSize = lowerRight.y - upperLeft.y;
                            const auto pByteBuffer = static_cast<char*>(pBuffer);
                            auto pData = pByteBuffer;
                            for (int bandIndex = 1; bandIndex <= bandCount; bandIndex++)
                            {
                                result = result && (band = dataset->GetRasterBand(bandIndex));
                                result = result && band->GetRasterDataType() != GDT_Unknown;
                                result = result && !band->GetColorTable();
                                result = result && band->RasterIO(GF_Read,
                                    dataOffset.x, dataOffset.y,
                                    dataSize.x, dataSize.y,
                                    pData, tileSize, tileSize,
                                    destDataType, 0, 0, &extraArg) == CE_None;
                                if (!result) break;
                                pData += tileSize * tileSize * pixelSizeInBytes;
                            }
                            if (result && destDataType == GDT_Float32 && bufferSize >= 16)
                            {
                                // Check values ​​in the center and corners to know that data is present
                                const auto noData = static_cast<float>(band->GetNoDataValue());
                                const auto side = tileSize * pixelSizeInBytes;
                                const auto halfSize = tileSize >> 1;
                                const auto middle = halfSize * halfSize * pixelSizeInBytes;
                                result = *(reinterpret_cast<const float*>(pByteBuffer + middle)) != noData;
                                result = result &&
                                    *(reinterpret_cast<const float*>(pByteBuffer)) != noData;
                                result = result &&
                                    *(reinterpret_cast<const float*>(pByteBuffer + side - 4)) != noData;
                                result = result &&
                                    *(reinterpret_cast<const float*>(pByteBuffer + bufferSize - 4)) != noData;
                                result = result &&
                                    *(reinterpret_cast<const float*>(pByteBuffer + bufferSize - side)) != noData;

                                // Check value in the center of top overscaled tile
                                if (result && zoomShift > 0 && minZoom > MinZoomLevel)
                                {
                                    upperLeft = metersFrom31(upperLeftOverscaled.x, upperLeftOverscaled.y);
                                    lowerRight = metersFrom31(lowerRightOverscaled.x, lowerRightOverscaled.y);
                                    upperLeft.x = (upperLeft.x - geoTransform[0]) / geoTransform[1];
                                    upperLeft.y = (upperLeft.y - geoTransform[3]) / geoTransform[5];
                                    lowerRight.x = (lowerRight.x - geoTransform[0]) / geoTransform[1];
                                    lowerRight.y = (lowerRight.y - geoTransform[3]) / geoTransform[5];
                                    extraArg.dfXOff = (upperLeft.x + lowerRight.x) * 0.5;
                                    extraArg.dfYOff = (upperLeft.y + lowerRight.y) * 0.5;
                                    extraArg.dfXSize = 1.0;
                                    extraArg.dfYSize = 1.0;
                                    band = dataset->GetRasterBand(1);
                                    const auto cornerValue = new float[1];
                                    result = band->RasterIO(GF_Read,
                                        std::floor(extraArg.dfXOff), std::floor(extraArg.dfYOff),
                                        1, 1, cornerValue, 1, 1, destDataType, 0, 0, &extraArg) == CE_None;
                                    result = result && cornerValue[0] != noData;
                                    delete[] cornerValue;
                                }                                
                            }
                            if (result && cacheDatabase)
                            {
                                // Put raster data in cache file
                                if (cacheDatabase->isOpened())
                                    cacheDatabase->storeTileData(tileId, zoom, specification,
                                        QByteArray::fromRawData(pByteBuffer, bufferSize));
                            }
                        }
                        GDALClose(dataset);
                    }
                    if (result)
                        return true;
                }
            }
        }
    }

    return false;
}

void OsmAnd::GeoTiffCollection_P::onDirectoryChanged(const QString& path)
{
    invalidateCollectedSources();
}

void OsmAnd::GeoTiffCollection_P::onFileChanged(const QString& path)
{
    invalidateCollectedSources();
}
