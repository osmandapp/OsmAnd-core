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
    _minZoom.storeRelease(ZoomLevel10);
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

inline double OsmAnd::GeoTiffCollection_P::metersTo31(const double positionInMeters) const
{
    return qBound(0.0, (positionInMeters / earthInMeters + 0.5) * earthIn31, static_cast<double>(INT32_MAX));
}

inline OsmAnd::PointD OsmAnd::GeoTiffCollection_P::metersTo31(const PointD& locationInMeters) const
{
    return PointD(metersTo31(locationInMeters.x), metersTo31(locationInMeters.y));
}

inline double OsmAnd::GeoTiffCollection_P::metersFrom31(const double position31) const
{
    return (qBound(0.0, position31, static_cast<double>(INT32_MAX)) / earthIn31 - 0.5) * earthInMeters;
}

inline OsmAnd::PointD OsmAnd::GeoTiffCollection_P::metersFrom31(const PointD& location31) const
{
    return PointD(metersFrom31(location31.x), metersFrom31(location31.y));
}

inline OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::calcMaxZoom(
    const int32_t pixelSize31, const uint32_t tileSize) const
{
    const auto tileSize31 =
        log2(qBound(1.0, (static_cast<double>(pixelSize31) + 1.0) * tileSize, static_cast<double>(INT32_MAX) + 1.0));
    return static_cast<ZoomLevel>(MaxZoomLevel - std::ceil(tileSize31));
}

inline OsmAnd::GeoTiffCollection_P::GeoTiffProperties OsmAnd::GeoTiffCollection_P::getGeoTiffProperties(
    const QString& filePath) const
{   
    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filePath), GA_ReadOnly))
    {
        const auto rasterBandCount = dataset->GetRasterCount();
        auto bandDataType = GDT_Unknown;
        GDALRasterBand* band;
        bool result = rasterBandCount > 0 && rasterBandCount < 5 && (band = dataset->GetRasterBand(1));
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
                PointD upperLeft = metersTo31(PointD(geoTransform[0], geoTransform[3]));
                PointD lowerRight = metersTo31(PointD(
                    geoTransform[0] + geoTransform[1] * (dataset->GetRasterXSize() - 1),
                    geoTransform[3] + geoTransform[5] * (dataset->GetRasterYSize() - 1)));
                upperLeft.y = static_cast<double>(INT32_MAX) - upperLeft.y;
                lowerRight.y = static_cast<double>(INT32_MAX) - lowerRight.y;
                const auto pixelSize31 =
                    qBound(0.0, earthIn31 / earthInMeters * geoTransform[1], static_cast<double>(INT32_MAX));
                TileSqliteDatabase::Meta meta;
                if (!cacheDb->obtainMeta(meta))
                {
                    meta.setMinZoom(getMinZoom());
                    meta.setMaxZoom(MaxZoomLevel);
                    meta.setTileNumbering(QStringLiteral(""));
                    meta.setSpecificated(QStringLiteral("yes"));
                    cacheDb->storeMeta(meta);
                }
                return GeoTiffProperties(upperLeft, lowerRight, pixelSize31, rasterBandCount, bandDataType, cacheDb);
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
    const auto intMax = static_cast<double>(INT32_MAX);
    auto zoomDelta = MaxZoomLevel - zoom;
    int maxIndex = ~(-1u << zoomDelta);
    auto overlapOffset = (static_cast<double>(maxIndex) + 1.0) * 0.5 *
        static_cast<double>(overlap) / static_cast<double>(tileSize - overlap);
    PointI upperLeft31(tileId.x << zoomDelta, tileId.y << zoomDelta);
    PointI lowerRight31(upperLeft31.x | maxIndex, upperLeft31.y | maxIndex);
    PointD upperLeft(
        qMax(0.0, static_cast<double>(upperLeft31.x) - overlapOffset),
        qMax(0.0, static_cast<double>(upperLeft31.y) - overlapOffset));
    PointD lowerRight(
        qMin(intMax, static_cast<double>(lowerRight31.x) + 1.0 + overlapOffset),
        qMin(intMax, static_cast<double>(lowerRight31.y) + 1.0 + overlapOffset));

    // Calculate data properties
    const auto specification = tileSize * 100 + overlap * 10 + (toBytes ? bandCount : bandCount + 4);
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
                tiffProperties.region31.contains(AreaD(upperLeft, lowerRight)))
            {
                const int zoomShift = zoom - minZoom;
                bool tileFound = true;
                if (zoomShift > 0 && minZoom > MinZoomLevel)
                {
                    auto zoomDelta = MaxZoomLevel - minZoom;
                    maxIndex = ~(-1u << zoomDelta);
                    auto overlapOffset = (static_cast<double>(maxIndex) + 1.0) * 0.5 *
                        static_cast<double>(overlap) / static_cast<double>(tileSize - overlap);
                    upperLeft31.x = tileId.x >> zoomShift << zoomDelta;
                    upperLeft31.y = tileId.y >> zoomShift << zoomDelta;
                    lowerRight31.x = upperLeft31.x | maxIndex;
                    lowerRight31.y = upperLeft31.y | maxIndex;
                    upperLeft.x = qMax(0.0, static_cast<double>(upperLeft31.x) - overlapOffset);
                    upperLeft.y = qMax(0.0, static_cast<double>(upperLeft31.y) - overlapOffset);
                    lowerRight.x = qMin(intMax, static_cast<double>(lowerRight31.x) + 1.0 + overlapOffset);
                    lowerRight.y = qMin(intMax, static_cast<double>(lowerRight31.y) + 1.0 + overlapOffset);
                    if (!tiffProperties.region31.contains(AreaD(upperLeft, lowerRight)))
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
                        result = rasterBandCount > 0 && rasterBandCount < 5 && rasterBandCount >= bandCount;
                        double geoTransform[6];
                        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
                            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
                        {
                            const auto zoomDelta = MaxZoomLevel - zoom;
                            const int maxIndex = ~(-1u << zoomDelta);
                            auto overlapOffset = (static_cast<double>(maxIndex) + 1.0) * 0.5 *
                                static_cast<double>(overlap) / static_cast<double>(tileSize - overlap);
                            const PointI upperLeft31(tileId.x << zoomDelta, tileId.y << zoomDelta);
                            const PointI lowerRight31(upperLeft31.x | maxIndex, upperLeft31.y | maxIndex);
                            PointD upperLeft(static_cast<double>(upperLeft31.x) - overlapOffset,
                                static_cast<double>(INT32_MAX) -
                                static_cast<double>(upperLeft31.y) + overlapOffset);
                            PointD lowerRight(static_cast<double>(lowerRight31.x) + 1.0 + overlapOffset,
                                static_cast<double>(INT32_MAX) -
                                static_cast<double>(lowerRight31.y) - 1.0 - overlapOffset);
                            upperLeft = metersFrom31(upperLeft);
                            lowerRight = metersFrom31(lowerRight);
                            upperLeft.x = (upperLeft.x - geoTransform[0]) / geoTransform[1];
                            upperLeft.y = (upperLeft.y - geoTransform[3]) / geoTransform[5];
                            lowerRight.x = (lowerRight.x - geoTransform[0]) / geoTransform[1];
                            lowerRight.y = (lowerRight.y - geoTransform[3]) / geoTransform[5];
                            GDALRasterIOExtraArg extraArg;
                            extraArg.nVersion = RASTERIO_EXTRA_ARG_CURRENT_VERSION;
                            extraArg.eResampleAlg = GRIORA_Average;
                            extraArg.pfnProgress = CPL_NULLPTR;
                            extraArg.pProgressData = CPL_NULLPTR;
                            extraArg.bFloatingPointWindowValidity = TRUE;
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
                                    std::floor(extraArg.dfXOff), std::floor(extraArg.dfYOff),
                                    std::ceil(extraArg.dfXSize), std::ceil(extraArg.dfYSize),
                                    pData, tileSize, tileSize,
                                    destDataType, 0, 0, &extraArg) == CE_None;
                                if (!result) break;
                                pData += tileSize * tileSize * pixelSizeInBytes;
                            }
                            if (result && destDataType == GDT_Float32 && bufferSize >= 8)
                            {
                                // Check the value in the middle to know the data is valid
                                const auto noData = static_cast<float>(band->GetNoDataValue());
                                pData = pByteBuffer + bufferSize / 8 * 4;
                                result = *(reinterpret_cast<const float*>(pData)) != noData;
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
