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
            itCollectedSources = _collectedSources.insert(originId, QHash<QString, AreaD>());
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
                collectedSources.insert(filePath, getGeoTiffRegion31(filePath));
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
            collectedSources.insert(filePath, getGeoTiffRegion31(filePath));
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

double OsmAnd::GeoTiffCollection_P::metersTo31(const double positionInMeters) const
{
    return qMax(0.0, std::ceil((positionInMeters / earthInMeters + 0.5) * earthIn31) - 1.0);
}

OsmAnd::PointD OsmAnd::GeoTiffCollection_P::metersTo31(const PointD& locationInMeters) const
{
    return PointD(metersTo31(locationInMeters.x), metersTo31(locationInMeters.y));
}

double OsmAnd::GeoTiffCollection_P::metersFrom31(const double position31) const
{
    return (qBound(0.0, position31, static_cast<double>(INT32_MAX)) / earthIn31 - 0.5) * earthInMeters;
}

OsmAnd::PointD OsmAnd::GeoTiffCollection_P::metersFrom31(const PointD& location31) const
{
    return PointD(metersFrom31(location31.x), metersFrom31(location31.y));
}

OsmAnd::AreaD OsmAnd::GeoTiffCollection_P::getGeoTiffRegion31(const QString& filename) const
{   
    PointD upperLeft(1.0, 1.0);
    PointD lowerRight(0.0, 0.0);
    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filename), GA_ReadOnly))
    {
        bool result = dataset->GetRasterCount() == 1;
        GDALRasterBand* band;
        result = result && (band = dataset->GetRasterBand(1));
        result = result && band->GetRasterDataType() == GDT_Int16;
        result = result && !band->GetColorTable();
        double geoTransform[6];
        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
        {
            upperLeft = metersTo31(PointD(geoTransform[0], geoTransform[3]));
            lowerRight = metersTo31(PointD(geoTransform[0] + geoTransform[1] * (dataset->GetRasterXSize() - 1),
                geoTransform[3] + geoTransform[5] * (dataset->GetRasterYSize() - 1)));
            upperLeft.y = static_cast<double>(INT32_MAX) - upperLeft.y;
            lowerRight.y = static_cast<double>(INT32_MAX) - lowerRight.y;
        }
        GDALClose(dataset);
    }
    return AreaD(upperLeft, lowerRight);
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

void OsmAnd::GeoTiffCollection_P::onDirectoryChanged(const QString& path)
{
    invalidateCollectedSources();
}

void OsmAnd::GeoTiffCollection_P::onFileChanged(const QString& path)
{
    invalidateCollectedSources();
}

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::getMinZoom() const
{
    return ZoomLevel10;
}

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection_P::getMaxZoom() const
{
    return ZoomLevel15;
}

QString OsmAnd::GeoTiffCollection_P::getGeoTiffFilename(
    const TileId& tileId, const ZoomLevel zoom, const uint32_t tileSize) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();
    
    const auto minZoom = getMinZoom();
    const auto maxZoom = getMaxZoom();
    if (zoom < minZoom || zoom > maxZoom)
        return "";
    const auto intMax = static_cast<double>(INT32_MAX);
    auto zoomDelta = MaxZoomLevel - zoom;
    int maxIndex = ~(-1u << zoomDelta);
    auto overlapOffset =
        (static_cast<double>(maxIndex) + 1.0) * 1.5 / static_cast<double>(tileSize - 3);
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
                const auto& fileRegion = itEntry.value();
                if (fileRegion.contains(AreaD(upperLeft, lowerRight)))
                {
                    const int zoomShift = zoom - minZoom;
                    if (zoomShift > 0)
                    {
                        auto zoomDelta = MaxZoomLevel - minZoom;
                        maxIndex = ~(-1u << zoomDelta);
                        overlapOffset =
                            (static_cast<double>(maxIndex) + 1.0) * 1.5 / static_cast<double>(tileSize - 3);
                        upperLeft31.x = tileId.x >> zoomShift << zoomDelta;
                        upperLeft31.y = tileId.y >> zoomShift << zoomDelta;
                        lowerRight31.x = upperLeft31.x | maxIndex;
                        lowerRight31.y = upperLeft31.y | maxIndex;
                        upperLeft.x = qMax(0.0, static_cast<double>(upperLeft31.x) - overlapOffset);
                        upperLeft.y = qMax(0.0, static_cast<double>(upperLeft31.y) - overlapOffset);
                        lowerRight.x = qMin(intMax, static_cast<double>(lowerRight31.x) + 1.0 + overlapOffset);
                        lowerRight.y = qMin(intMax, static_cast<double>(lowerRight31.y) + 1.0 + overlapOffset);
                        if (fileRegion.contains(AreaD(upperLeft, lowerRight)))
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

bool OsmAnd::GeoTiffCollection_P::getGeoTiffData(
    const QString& filename, const TileId& tileId, const ZoomLevel zoom, const uint32_t tileSize, void *pBuffer) const
{
    if (zoom < getMinZoom() || zoom > getMaxZoom())
        return false;
    bool result = false;
    if (const auto dataset = (GDALDataset *) GDALOpen(qPrintable(filename), GA_ReadOnly))
    {
        result = dataset->GetRasterCount() == 1;
        GDALRasterBand* band;
        result = result && (band = dataset->GetRasterBand(1));
        result = result && band->GetRasterDataType() == GDT_Int16;
        result = result && !band->GetColorTable();
        double geoTransform[6];
        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
        {
            const auto zoomDelta = MaxZoomLevel - zoom;
            const int maxIndex = ~(-1u << zoomDelta);
            const auto overlapOffset =
                (static_cast<double>(maxIndex) + 1.0) * 1.5 / static_cast<double>(tileSize - 3);
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
            result = band->RasterIO(GF_Read,
                std::floor(extraArg.dfXOff), std::floor(extraArg.dfYOff),
                std::ceil(extraArg.dfXSize), std::ceil(extraArg.dfYSize),
                pBuffer, tileSize, tileSize,
                GDT_Float32, 0, 0, &extraArg) == CE_None;
        }
        GDALClose(dataset);
    }
    return result;
}
