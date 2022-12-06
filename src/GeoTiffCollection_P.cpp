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
            itCollectedSourcesEntry.remove();
            continue;
        }

        // Check for missing files
        auto itFileEntry = mutableIteratorOf(collectedSources);
        while(itFileEntry.hasNext())
        {
            const auto& sourceFilename = itFileEntry.next().key();
            if (QFile::exists(sourceFilename))
                continue;
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
    const QString& filename) const
{   
    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filename), GA_ReadOnly))
    {
        const auto rasterBandCount = dataset->GetRasterCount();
        auto bandDataType = GDT_Unknown;
        GDALRasterBand* band;
        bool result = rasterBandCount > 0 && (band = dataset->GetRasterBand(1));
        result = result && (bandDataType = band->GetRasterDataType()) != GDT_Unknown;
        result = result && !band->GetColorTable();
        double geoTransform[6];
        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
        {
            PointD upperLeft = metersTo31(PointD(geoTransform[0], geoTransform[3]));
            PointD lowerRight = metersTo31(PointD(geoTransform[0] + geoTransform[1] * (dataset->GetRasterXSize() - 1),
                geoTransform[3] + geoTransform[5] * (dataset->GetRasterYSize() - 1)));
            upperLeft.y = static_cast<double>(INT32_MAX) - upperLeft.y;
            lowerRight.y = static_cast<double>(INT32_MAX) - lowerRight.y;
            const auto pixelSize31 =
                qBound(0.0, earthIn31 / earthInMeters * geoTransform[1], static_cast<double>(INT32_MAX));
            GDALClose(dataset);
            return GeoTiffProperties(upperLeft, lowerRight, pixelSize31, rasterBandCount, bandDataType);
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
            const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(sourceOrigin);

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

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::getMaxZoom(const uint32_t tileSize) const
{
    int32_t pixelSize31;
    if ((pixelSize31 = _pixelSize31.loadAcquire()) > 0)
    {
        return calcMaxZoom(pixelSize31, tileSize);
    }

    auto maxZoom = InvalidZoomLevel;
    {
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
    }
    if (pixelSize31 > 0)
        _pixelSize31.storeRelease(pixelSize31);

    return maxZoom;
}

QString OsmAnd::GeoTiffCollection_P::getGeoTiffFilename(const TileId& tileId, const ZoomLevel zoom,
    const uint32_t tileSize, const uint32_t overlap, const ZoomLevel minZoom /*= MinZoomLevel*/) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();
    
    const auto maxZoom = getMaxZoom(tileSize - overlap);
    if (zoom < minZoom || zoom > maxZoom)
        return "";
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
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);
        
        for (const auto& collectedSources : constOf(_collectedSources))
        {
            for (const auto& itEntry : rangeOf(constOf(collectedSources)))
            {
                const auto& fileName = itEntry.key();
                const auto& tiffProperties = itEntry.value();
                if (tiffProperties.region31.contains(AreaD(upperLeft, lowerRight)))
                {
                    const int zoomShift = zoom - minZoom;
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
                        if (tiffProperties.region31.contains(AreaD(upperLeft, lowerRight)))
                            return QString(fileName);
                    }
                    else
                        return QString(fileName);
                }
            }
        }
    }

    return "";
}

bool OsmAnd::GeoTiffCollection_P::getGeoTiffData(const QString& filename, const TileId& tileId, const ZoomLevel zoom,
    const uint32_t tileSize, const uint32_t overlap, void *pBuffer) const
{
    if (zoom > getMaxZoom(tileSize - overlap))
        return false;
    bool result = false;
    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filename), GA_ReadOnly))
    {
        const auto rasterBandCount = dataset->GetRasterCount();
        GDALRasterBand* band;
        result = rasterBandCount > 0;
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
                static_cast<double>(INT32_MAX) - static_cast<double>(upperLeft31.y) + overlapOffset);
            PointD lowerRight(static_cast<double>(lowerRight31.x) + 1.0 + overlapOffset,
                static_cast<double>(INT32_MAX) - static_cast<double>(lowerRight31.y) - 1.0 - overlapOffset);
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
            const auto destDataType = rasterBandCount > 1 ? GDT_Byte : GDT_Float32;
            const auto pixelSizeInBytes = destDataType == GDT_Byte ? 1 : 4;
            auto pByteBuffer = static_cast<char*>(pBuffer);
            for (int bandIndex = 1; bandIndex <= rasterBandCount; bandIndex++)
            {
                result = result && (band = dataset->GetRasterBand(bandIndex));
                result = result && band->GetRasterDataType() != GDT_Unknown;
                result = result && !band->GetColorTable();
                result = result && band->RasterIO(GF_Read,
                    std::floor(extraArg.dfXOff), std::floor(extraArg.dfYOff),
                    std::ceil(extraArg.dfXSize), std::ceil(extraArg.dfYSize),
                    pByteBuffer, tileSize, tileSize,
                    destDataType, 0, 0, &extraArg) == CE_None;
                if (!result) break;
                pByteBuffer += tileSize * tileSize * pixelSizeInBytes;
            }
        }
        GDALClose(dataset);
    }
    return result;
}

void OsmAnd::GeoTiffCollection_P::onDirectoryChanged(const QString& path)
{
    invalidateCollectedSources();
}

void OsmAnd::GeoTiffCollection_P::onFileChanged(const QString& path)
{
    invalidateCollectedSources();
}
