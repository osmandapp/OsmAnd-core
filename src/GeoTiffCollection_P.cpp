#include "GeoTiffCollection_P.h"
#include "GeoTiffCollection.h"

#include <cassert>

#include "ignore_warnings_on_external_includes.h"
#include <gdal_priv.h>
#include <gdal_utils.h>
#include <cpl_conv.h>
#include "restore_internal_warnings.h"

#include "QtCommon.h"

#include "IMapElevationDataProvider.h"
#include "OsmAndCore_private.h"
#include "QKeyValueIterator.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

#ifndef TIFF_NODATA
#define TIFF_NODATA (-32768.0)
#endif

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

inline bool OsmAnd::GeoTiffCollection_P::isPartOfTile(
    const AreaI& region31, const AreaI& area31) const
{
    if (region31.intersects(area31))
        return true;
    if (area31.left() >= 0 && region31.intersects(AreaI(
        area31.top(),
        area31.left() - INT32_MAX - 1,
        area31.bottom(),
        area31.right() - INT32_MAX - 1)))
        return true;
    if (area31.top() >= 0 && region31.intersects(AreaI(
        area31.top() - INT32_MAX - 1,
        area31.left(),
        area31.bottom() - INT32_MAX - 1,
        area31.right())))
        return true;
    if (area31.left() >= 0 && area31.top() >= 0 && region31.intersects(AreaI(
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
    if (const auto dataset = (GDALDataset*) GDALOpen(qPrintable(filePath), GA_ReadOnly))
    {
        const auto bandCount = dataset->GetRasterCount();
        auto bandDataType = GDT_Unknown;
        GDALRasterBand* band;
        bool result = bandCount > 0 && bandCount < 10 && (band = dataset->GetRasterBand(1));
        result = result && (bandDataType = band->GetRasterDataType()) != GDT_Unknown;
        result = result && !band->GetColorTable();
        double geoTransform[6];
        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
        {
            auto upperLeft31 = metersTo31(PointD(geoTransform[0], geoTransform[3]));
            const auto rasterSize = PointI(dataset->GetRasterXSize(), dataset->GetRasterYSize());
            auto lowerRight31 = metersTo31(PointD(
                geoTransform[0] + geoTransform[1] * rasterSize.x,
                geoTransform[3] + geoTransform[5] * rasterSize.y));
            if (upperLeft31.x > lowerRight31.x)
                upperLeft31.x = upperLeft31.x - INT32_MAX - 1;
            if (upperLeft31.y > lowerRight31.y)
                upperLeft31.y = upperLeft31.y - INT32_MAX - 1;
            const int32_t pixelSize31 =
                std::ceil(qBound(0.0, geoTransform[1] * earthIn31 / earthInMeters, earthIn31 - 1.0));
            GDALClose(dataset);
            return GeoTiffProperties(upperLeft31, lowerRight31, rasterSize, pixelSize31, bandCount, bandDataType);
        }
        GDALClose(dataset);
    }
    return GeoTiffProperties();
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::GeoTiffCollection_P::openCacheFile(const QString filename)
{
    QMutexLocker scopedLocker(&_localCacheDirMutex);

    const auto dbFilePath = (_localCacheDir.path().isEmpty() ? QStringLiteral(".") :
        _localCacheDir.path()) + QDir::separator() + filename;
    const auto cacheDb = std::make_shared<TileSqliteDatabase>(dbFilePath);
    if (cacheDb->open(true))
    {
        TileSqliteDatabase::Meta meta;
        if (!cacheDb->obtainMeta(meta))
        {
            meta.setMinZoom(MinZoomLevel);
            meta.setMaxZoom(MaxZoomLevel);
            meta.setTileNumbering(QStringLiteral(""));
            meta.setSpecificated(QStringLiteral("yes"));
            cacheDb->storeMeta(meta);
            cacheDb->enableTileTimeSupport();
            cacheDb->enableValueRangeSupport();
        }
        return cacheDb;
    }
    return nullptr;
}

inline bool OsmAnd::GeoTiffCollection_P::isDataPresent(
    const char* pByteOffset, const GDALDataType dataType, const double noData) const
{
    double value;
    switch (dataType)
    {
    case GDT_Float32:
        value = *(reinterpret_cast<const float*>(pByteOffset));
        break;
    case GDT_Byte:
        value = *pByteOffset;
        break;
    case GDT_Int16:
        value = *(reinterpret_cast<const short*>(pByteOffset));
        break;
    default:
        value = noData;
    }
    return value != noData;
}

inline void OsmAnd::GeoTiffCollection_P::getMinMaxValues(const char* pByteBuffer, const GDALDataType dataType,
    const double noData, const uint32_t valueCount, float& minValue, float& maxValue) const
{
    const auto noDataFloat = static_cast<float>(noData);
    const auto valueSizeInBytes = dataType == GDT_Byte ? 1 : (dataType == GDT_Int16 ? 2 : 4);
    auto minData = minValue;
    auto maxData = maxValue;
    float value;
    auto pValues = pByteBuffer;
    const auto pStop = pValues + valueCount * valueSizeInBytes;
    while (pValues != pStop)
    {
        switch (dataType)
        {
        case GDT_Float32:
            value = *(reinterpret_cast<const float*>(pValues));
            break;
        case GDT_Byte:
            value = *pValues;
            break;
        case GDT_Int16:
            value = *(reinterpret_cast<const short*>(pValues));
            break;
        default:
            return;
        }
        if (value != noDataFloat)
        {
            if (value < minData)
                minData = value;
            if (value > maxData)
                maxData = value;
        }
        pValues += valueSizeInBytes;
    }
    minValue = minData;
    maxValue = maxData;
}

inline uint64_t OsmAnd::GeoTiffCollection_P::multiplyParts(const uint64_t shade, const uint64_t slope) const
{
    const uint64_t mask0 = 0xFFULL;
    const uint64_t mask1 = 0xFF0000ULL;
    const uint64_t mask2 = 0xFF00000000ULL;
    const uint64_t mask3 = 0xFF000000000000ULL;
    return ((shade & mask0) != 0 ? (shade & mask0) * (slope & mask0) + 0x0100ULL : 0) |
        ((shade & mask1) != 0 ? (shade & mask1) * ((slope & mask1) >> 16) + 0x01000000ULL : 0) |
        ((shade & mask2) != 0 ? (shade & mask2) * ((slope & mask2) >> 32) + 0x010000000000ULL : 0) |
        ((shade & mask3) != 0 ? (shade & mask3) * (slope >> 48) + 0x0100000000000000ULL : 0);
}

inline void OsmAnd::GeoTiffCollection_P::blendHillshade(
    const uint32_t tileSize,
    uchar* shade,
    uchar* slope,
    uchar* blend) const
{
    if ((tileSize & 7) > 0)
    {
        // Simple pixel-by-pixel blending
        const auto pixelCount = tileSize * tileSize;
        for (uint32_t idx = 0; idx < pixelCount; idx++)
            blend[idx] =
                shade[idx] != 0 ? (static_cast<ushort>(shade[idx]) * static_cast<ushort>(slope[idx]) >> 8) + 1 : 0;
    }
    else
    {
        // Optimized blending
        const uint64_t bigMask = 0xFF00FF00FF00FF00ULL;
        const uint64_t litMask = 0x00FF00FF00FF00FFULL;
        const auto quadCount = tileSize * tileSize / 8;
        auto pShade = reinterpret_cast<uint64_t*>(shade);
        auto pSlope = reinterpret_cast<uint64_t*>(slope);
        auto pBlend = reinterpret_cast<uint64_t*>(blend);
        for (int i = 0; i < quadCount; i++)
        {
            auto quad = *pShade++;
            auto hsb = (quad & bigMask) >> 8;
            auto hsl = quad & litMask;
            quad = *pSlope++;
            auto spb = (quad & bigMask) >> 8;
            auto spl = quad & litMask;
            *pBlend = (multiplyParts(hsb, spb) & bigMask) | (multiplyParts(hsl, spl) >> 8 & litMask);
            pBlend++;
        }
    }
}

inline void OsmAnd::GeoTiffCollection_P::mergeHeights(
    const uint32_t tileSize,
    const float scaleFactor,
    const bool forceReplace,
    QByteArray& destination,
    float* source) const
{
    const auto noData = static_cast<float>(TIFF_NODATA);
    auto result = reinterpret_cast<float*>(destination.data());
    const auto valueCount = tileSize * tileSize;
    for (uint32_t idx = 0; idx < valueCount; idx++)
    {
        if (forceReplace || result[idx] == noData)
        {
            const auto src = source[idx];
            result[idx] = src == noData ? noData : src * scaleFactor;
        }
    }
}

inline bool OsmAnd::GeoTiffCollection_P::postProcess(
    const char* pByteBuffer,
    const GeoTiffCollection::ProcessingParameters& procParameters,
    const uint32_t tileSize,
    const uint32_t overlap,
    const PointD& tileOrigin,
    const PointD& tileResolution,
    const int gcpCount,
    const void* gcpList,
    void* pBuffer) const
{
    bool result = false;
    char dataPointer[24];
    auto pCharBuffer = const_cast<char*>(pByteBuffer);
    auto pGCPList = reinterpret_cast<const GDAL_GCP*>(gcpList);
    dataPointer[CPLPrintPointer(dataPointer, pCharBuffer, 23)] = 0;
    auto filename = new char[1024];
    sprintf(filename,
        "MEM:::DATATYPE=Float32,DATAPOINTER=%s,PIXELS=%d,LINES=%d,GEOTRANSFORM=%.17g/%.17g/0.0/%.17g/0.0/%.17g",
        dataPointer, tileSize, tileSize, tileOrigin.x, tileResolution.x, tileOrigin.y, tileResolution.y);
    if (const auto heightmapDataset = (GDALDataset*) GDALOpen(filename, GA_ReadOnly))
    {
        auto band = heightmapDataset->GetRasterBand(1);
        if (band && heightmapDataset->SetGCPs(gcpCount, pGCPList, "") == CE_None)
        {
            band->SetNoDataValue(TIFF_NODATA);
            // Prepare common slope raster
            char outputFormat[] = "MEM";
            const char* slopeArgs[] = { "-of", outputFormat, NULL };
            if (GDALDEMProcessingOptions* slopeOptions = GDALDEMProcessingOptionsNew(
                const_cast<char **>(slopeArgs), NULL))
            {
                if (const auto slopeDataset = (GDALDataset*) GDALDEMProcessing(outputFormat, heightmapDataset,
                    "slope", nullptr, slopeOptions, nullptr))
                {
                    if (procParameters.rasterType == GeoTiffCollection::RasterType::Hillshade)
                    {
                        // Compose hybrid hillshade raster
                        const char* hillshadeArgs[] = { "-of", outputFormat, "-z", "2", NULL };
                        if (GDALDEMProcessingOptions* hillshadeOptions = GDALDEMProcessingOptionsNew(
                            const_cast<char **>(hillshadeArgs), NULL))
                        {
                            const auto hillshadeDataset = (GDALDataset*) GDALDEMProcessing(
                                outputFormat,
                                heightmapDataset,
                                "hillshade",
                                nullptr,
                                hillshadeOptions,
                                nullptr);
                            if (hillshadeDataset)
                            {
                                const auto grayscaleDataset = (GDALDataset*) GDALDEMProcessing(
                                    outputFormat,
                                    slopeDataset,
                                    "color-relief",
                                    qPrintable(procParameters.intermediateColorsFilename),
                                    slopeOptions,
                                    nullptr);
                                if (grayscaleDataset)
                                {
                                    const auto resultOffset = overlap / 2;
                                    const auto resultSize = tileSize - overlap;
                                    const auto resultLength = resultSize * resultSize;
                                    auto gsBuffer = new uchar[resultLength];
                                    GDALRasterBand* gsBand = grayscaleDataset->GetRasterBand(1);
                                    if (gsBand->RasterIO(GF_Read, resultOffset, resultOffset, resultSize, resultSize,
                                        gsBuffer, resultSize, resultSize, GDT_Byte, 0, 0, nullptr) == CE_None)
                                    {
                                        auto hsBuffer = new uchar[resultLength];
                                        GDALRasterBand* band = hillshadeDataset->GetRasterBand(1);
                                        if (band->RasterIO(GF_Read, resultOffset, resultOffset, resultSize, resultSize,
                                            hsBuffer, resultSize, resultSize, GDT_Byte, 0, 0, nullptr) == CE_None)
                                        {
                                            auto mixBuffer = new uchar[resultLength];
                                            blendHillshade(resultSize, hsBuffer, gsBuffer, mixBuffer);
                                            dataPointer[CPLPrintPointer(dataPointer, mixBuffer, 23)] = 0;
                                            sprintf(filename,"MEM:::DATATYPE=Byte,DATAPOINTER=%s,PIXELS=%d,LINES=%d,"
                                                "GEOTRANSFORM=%.17g/%.17g/0.0/%.17g/0.0/%.17g",
                                                dataPointer, resultSize, resultSize,
                                                tileOrigin.x, tileResolution.x, tileOrigin.y, tileResolution.y);
                                            if (const auto mixDataset = (GDALDataset*) GDALOpen(filename, GA_ReadOnly))
                                            {
                                                if (mixDataset->SetGCPs(gcpCount, pGCPList, "") == CE_None)
                                                {
                                                    const char* colorizeArgs[] = {"-of", outputFormat, "-alpha", NULL};
                                                    if (GDALDEMProcessingOptions* colorizeOptions =
                                                        GDALDEMProcessingOptionsNew(
                                                        const_cast<char **>(colorizeArgs), NULL))
                                                    {
                                                        const auto resultDataset = (GDALDataset*) GDALDEMProcessing(
                                                            outputFormat,
                                                            mixDataset,
                                                            "color-relief",
                                                            qPrintable(procParameters.resultColorsFilename),
                                                            colorizeOptions,
                                                            nullptr);
                                                        if (resultDataset)
                                                        {
                                                            result = resultDataset->RasterIO(GF_Read,
                                                                0, 0, resultSize, resultSize,
                                                                pBuffer, resultSize, resultSize, GDT_Byte, 4, nullptr,
                                                                4, resultSize * 4, 1, nullptr) == CE_None;
                                                            GDALClose(resultDataset);                                                        
                                                        }
                                                        GDALDEMProcessingOptionsFree(colorizeOptions);
                                                    }
                                                }
                                                GDALClose(mixDataset);
                                            }
                                            delete[] mixBuffer;
                                        }
                                        delete[] hsBuffer;
                                    }
                                    delete[] gsBuffer;
                                    GDALClose(grayscaleDataset);
                                }
                                GDALClose(hillshadeDataset);
                            }
                            GDALDEMProcessingOptionsFree(hillshadeOptions);
                        }
                    }
                    else // GeoTiffCollection::RasterType::Slope
                    {
                        // Produce colorized slope raster
                        const char* colorizeArgs[] = { "-of", outputFormat, "-alpha", NULL };
                        if (GDALDEMProcessingOptions* colorizeOptions = GDALDEMProcessingOptionsNew(
                            const_cast<char **>(colorizeArgs), NULL))
                        {
                            const auto resultDataset = (GDALDataset*) GDALDEMProcessing(
                                outputFormat,
                                slopeDataset,
                                "color-relief",
                                qPrintable(procParameters.resultColorsFilename),
                                colorizeOptions,
                                nullptr);
                            if (resultDataset)
                            {
                                const auto resultOffset = overlap / 2;
                                const auto resultSize = tileSize - overlap;
                                result = resultDataset->RasterIO(GF_Read,
                                    resultOffset, resultOffset, resultSize, resultSize,
                                    pBuffer, resultSize, resultSize, GDT_Byte, 4, nullptr,
                                    4, resultSize * 4, 1, nullptr) == CE_None;
                                GDALClose(resultDataset);
                            }
                            GDALDEMProcessingOptionsFree(colorizeOptions);
                        }
                    }
                    GDALClose(slopeDataset);
                }
                GDALDEMProcessingOptionsFree(slopeOptions);
            }
        }
        GDALClose(heightmapDataset);
    }
    delete[] filename;
    return result;
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
        {
            QMutexLocker scopedLocker2(&_localCacheDirMutex);
            _localCacheDir = localCacheDir;
        }
        heightmapCache = openCacheFile(QStringLiteral("heightmap_cache.sqlite"));
        hillshadeCache = openCacheFile(QStringLiteral("hillshade_cache.sqlite"));
        slopeCache = openCacheFile(QStringLiteral("slope_cache.sqlite"));
    }

    clearCollectedSources();

    invalidateCollectedSources();
}

bool OsmAnd::GeoTiffCollection_P::removeFileTilesFromCache(
    const GeoTiffCollection::RasterType cache, const QString& filePath)
{
    const auto hash = qHash(filePath);
    const auto specification = *(reinterpret_cast<const int*>(&hash));
    std::shared_ptr<OsmAnd::TileSqliteDatabase> cacheDatabase;
    switch (cache)
    {
        case GeoTiffCollection::RasterType::Hillshade:
            cacheDatabase = hillshadeCache;
            break;
        case GeoTiffCollection::RasterType::Slope:
            cacheDatabase = slopeCache;
            break;
        default:
            cacheDatabase = heightmapCache;
    }
    if (cacheDatabase)
    {
        cacheDatabase->removeBiggerTilesData(getMinZoom());
        return cacheDatabase->removeSpecificTilesData(specification);
    }
    return false;
}

bool OsmAnd::GeoTiffCollection_P::refreshTilesInCache(const GeoTiffCollection::RasterType cache)
{
    std::shared_ptr<OsmAnd::TileSqliteDatabase> cacheDatabase;
    switch (cache)
    {
        case GeoTiffCollection::RasterType::Hillshade:
            cacheDatabase = hillshadeCache;
            break;
        case GeoTiffCollection::RasterType::Slope:
            cacheDatabase = slopeCache;
            break;
        default:
            cacheDatabase = heightmapCache;
    }
    if (cacheDatabase)
        cacheDatabase->removeBiggerTilesData(getMinZoom());
    return false;
}

bool OsmAnd::GeoTiffCollection_P::removeOlderTilesFromCache(
    const GeoTiffCollection::RasterType cache, const int64_t time)
{
    std::shared_ptr<OsmAnd::TileSqliteDatabase> cacheDatabase;
    switch (cache)
    {
        case GeoTiffCollection::RasterType::Hillshade:
            cacheDatabase = hillshadeCache;
            break;
        case GeoTiffCollection::RasterType::Slope:
            cacheDatabase = slopeCache;
            break;
        default:
            cacheDatabase = heightmapCache;
    }
    if (cacheDatabase)
        return cacheDatabase->removeOlderTilesData(time);
    return false;
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

    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

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

OsmAnd::GeoTiffCollection::CallResult OsmAnd::GeoTiffCollection_P::getGeoTiffData(
    const TileId& tileId,
    const ZoomLevel zoom,
    const uint32_t tileSize,
    const uint32_t overlap,
    const uint32_t bandCount,
    const bool toBytes,
    float& minValue,
    float& maxValue,
    void* pBuffer,
    const GeoTiffCollection::ProcessingParameters* procParameters) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();
    
    const auto minZoom = getMinZoom();
    const auto maxZoom = getMaxZoom(tileSize - overlap);
    const auto valueCount = tileSize * tileSize;

    // Use suitable downsampling algorithm
    auto resamplingAlgorithm = procParameters && zoom < minZoom ? GRIORA_Cubic : GRIORA_Average;

    // Composite tiles can be made up of data from multiple files
    QByteArray compositeTile;
    QByteArray compositeGCPList;
    int compositeGCPCount;
    int compositeSpecification;
    PointD compositeOrigin;
    PointD compositeResolution;
    bool compose = false;
    bool atLeastOnePresent = false;
    if (procParameters)
    {
        if (zoom > maxZoom)
            // Use suitable upsampling algorithm
            resamplingAlgorithm = GRIORA_CubicSpline;
        else if (zoom < minZoom)
        {
            compositeTile = QByteArray(valueCount * sizeof(float), 0);
            compose = true;
        }
    }
    else if (zoom < minZoom || zoom > maxZoom)
        return GeoTiffCollection::CallResult::Empty;

    // Calculate tile edges to find corresponding data
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
    lowerRight31.x = lowerRight64.x - 1;
    lowerRight31.y = lowerRight64.y - 1;

    PointD upperLeftOverscaled;
    PointD lowerRightOverscaled;

    GDALRasterIOExtraArg extraArg;
    extraArg.nVersion = RASTERIO_EXTRA_ARG_CURRENT_VERSION;
    extraArg.eResampleAlg = resamplingAlgorithm;
    extraArg.pfnProgress = CPL_NULLPTR;
    extraArg.pProgressData = CPL_NULLPTR;
    extraArg.bFloatingPointWindowValidity = TRUE;

    std::shared_ptr<OsmAnd::TileSqliteDatabase> cacheDatabase;

    const auto destDataType = toBytes ? GDT_Byte : GDT_Float32;
    auto pByteBuffer = static_cast<char*>(pBuffer);
    uint32_t bandSize;

    QReadLocker scopedLocker(&_collectedSourcesLock);

    // Files can have data for this tile
    bool available = false;

    // Some file failed to provide data for this tile
    bool incomplete = false;

    minValue = std::numeric_limits<float>::max();
    maxValue = std::numeric_limits<float>::lowest();
    double noData = TIFF_NODATA;
    for (const auto& collectedSources : constOf(_collectedSources))
    {
        for (const auto& itEntry : rangeOf(constOf(collectedSources)))
        {
            const auto& filePath = itEntry.key();
            const auto& tiffProperties = itEntry.value();
            if (compose && isPartOfTile(tiffProperties.region31, AreaI(upperLeft31, lowerRight31)) ||
                (!compose && containsTile(tiffProperties.region31, AreaI(upperLeft31, lowerRight31))))
            {
                available = true;
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
                    lowerRight31Overscaled.x = lowerRight64.x - 1;
                    lowerRight31Overscaled.y = lowerRight64.y - 1;
                    if (!containsTile(tiffProperties.region31, AreaI(upperLeft31Overscaled, lowerRight31Overscaled)))
                        tileFound = false;
                }

                if (tileFound)
                {
                    const auto hash = qHash(filePath);
                    const auto specification = *(reinterpret_cast<const int*>(&hash));
                        
                    // Check one time if data is available in cache file
                    if (!cacheDatabase)
                    {
                        if (procParameters)
                            cacheDatabase = procParameters->rasterType == GeoTiffCollection::RasterType::Hillshade
                                ? hillshadeCache : slopeCache;
                        else
                            cacheDatabase = heightmapCache;
                        if (cacheDatabase && cacheDatabase->isOpened() &&
                            cacheDatabase->obtainTileData(tileId, zoom, pBuffer, minValue, maxValue))
                        {
                            if (!procParameters && (minValue == std::numeric_limits<float>::max()
                                || maxValue == std::numeric_limits<float>::lowest()))
                            {
                                getMinMaxValues(pByteBuffer, destDataType, noData, valueCount, minValue, maxValue);
                            }
                            return GeoTiffCollection::CallResult::Completed;
                        }
                    }

                    // Tile is empty
                    bool empty = false;

                    bool result = false;
                    if (const auto dataset = (GDALDataset*) GDALOpen(qPrintable(filePath), GA_ReadOnly))
                    {
                        // Read raster data from source Geotiff file
                        const auto rasterBandCount = dataset->GetRasterCount();
                        GDALRasterBand* band;
                        result = rasterBandCount > 0 && rasterBandCount < 10;
                        double geoTransform[6];
                        if (result && dataset->GetGeoTransform(geoTransform) == CE_None &&
                            geoTransform[2] == 0.0 && geoTransform[4] == 0.0)
                        {
                            const auto gcpCount = dataset->GetGCPCount();
                            const auto gcpList = dataset->GetGCPs();
                            auto upperLeft = metersFrom31(upperLeftNative.x, upperLeftNative.y);
                            auto lowerRight = metersFrom31(lowerRightNative.x, lowerRightNative.y);
                            const auto tileOrigin = upperLeft;
                            const auto tileResolution = (lowerRight - upperLeft) / tileSize;
                            upperLeft.x = (upperLeft.x - geoTransform[0]) / geoTransform[1];
                            upperLeft.y = (upperLeft.y - geoTransform[3]) / geoTransform[5];
                            if (upperLeft.x > -1e-9)
                                upperLeft.x = qMax(upperLeft.x, 0.0);
                            if (upperLeft.y > -1e-9)
                                upperLeft.y = qMax(upperLeft.y, 0.0);
                            lowerRight.x = (lowerRight.x - geoTransform[0]) / geoTransform[1];
                            lowerRight.y = (lowerRight.y - geoTransform[3]) / geoTransform[5];
                            PointI dataOffset(std::floor(upperLeft.x), std::floor(upperLeft.y));
                            PointI dataSize(
                                static_cast<int32_t>(std::ceil(lowerRight.x)) - dataOffset.x,
                                static_cast<int32_t>(std::ceil(lowerRight.y)) - dataOffset.y);
                            PointI tileOffset(0, 0);
                            PointI tileLength(tileSize, tileSize);
                            const bool outRaster = dataOffset.x < 0 || dataOffset.y < 0 ||
                                dataOffset.x + dataSize.x > tiffProperties.rasterSize.x ||
                                dataOffset.y + dataSize.y > tiffProperties.rasterSize.y;
                            if (outRaster)
                            {
                                const double resSize = tileSize;
                                const PointD resFactor(
                                    (lowerRight.x - upperLeft.x) / resSize,
                                    (lowerRight.y - upperLeft.y) / resSize);
                                tileOffset.x = std::ceil(qMax(-upperLeft.x, 0.0) / resFactor.x);
                                tileOffset.y = std::ceil(qMax(-upperLeft.y, 0.0) / resFactor.y);
                                if (tileOffset.x > 0)
                                {
                                    tileLength.x -= tileOffset.x;
                                    upperLeft.x += static_cast<double>(tileOffset.x) * resFactor.x;
                                    dataOffset.x = std::floor(upperLeft.x);
                                    dataSize.x = static_cast<int32_t>(std::ceil(lowerRight.x)) - dataOffset.x;
                                }
                                if (tileOffset.y > 0)
                                {
                                    tileLength.y -= tileOffset.y;
                                    upperLeft.y += static_cast<double>(tileOffset.y) * resFactor.y;
                                    dataOffset.y = std::floor(upperLeft.y);
                                    dataSize.y = static_cast<int32_t>(std::ceil(lowerRight.y)) - dataOffset.y;
                                }
                                if (dataOffset.x + dataSize.x > tiffProperties.rasterSize.x)
                                {
                                    tileLength.x = tiffProperties.rasterSize.x - dataOffset.x;
                                    tileLength.x = std::floor(static_cast<double>(tileLength.x) / resFactor.x);
                                    lowerRight.x = upperLeft.x + static_cast<double>(tileLength.x) * resFactor.x;
                                    dataSize.x = static_cast<int32_t>(std::ceil(lowerRight.x)) - dataOffset.x;
                                    if (dataOffset.x + dataSize.x > tiffProperties.rasterSize.x)
                                    {
                                        dataSize.x--;
                                        lowerRight.x = dataOffset.x + dataSize.x;
                                    }
                                }
                                if (dataOffset.y + dataSize.y > tiffProperties.rasterSize.y)
                                {
                                    tileLength.y = tiffProperties.rasterSize.y - dataOffset.y;
                                    tileLength.y = std::floor(static_cast<double>(tileLength.y) / resFactor.y);
                                    lowerRight.y = upperLeft.y + static_cast<double>(tileLength.y) * resFactor.y;
                                    dataSize.y = static_cast<int32_t>(std::ceil(lowerRight.y)) - dataOffset.y;
                                    if (dataOffset.y + dataSize.y > tiffProperties.rasterSize.y)
                                    {
                                        dataSize.y--;
                                        lowerRight.y = dataOffset.y + dataSize.y;
                                    }
                                }
                            }
                            extraArg.dfXOff = upperLeft.x;
                            extraArg.dfYOff = upperLeft.y;
                            extraArg.dfXSize = lowerRight.x - upperLeft.x;
                            extraArg.dfYSize = lowerRight.y - upperLeft.y;
                            auto numOfBands = bandCount;
                            auto dataType = destDataType;
                            if (procParameters)
                            {
                                pByteBuffer = new char[valueCount * sizeof(float)];
                                numOfBands = 1;
                                dataType = GDT_Float32;
                            }
                            const auto pixelSizeInBytes = dataType == GDT_Byte ? 1 : (dataType == GDT_Int16 ? 2 : 4);
                            bandSize = valueCount * pixelSizeInBytes;
                            auto pData = pByteBuffer;
                            int pShift = outRaster ? (tileOffset.y * tileSize + tileOffset.x) * pixelSizeInBytes : 0;
                            const auto side = tileSize * pixelSizeInBytes;
                            for (int bandIndex = 1; bandIndex <= numOfBands; bandIndex++)
                            {
                                result = result && (band = dataset->GetRasterBand(bandIndex));
                                result = result && band->GetRasterDataType() != GDT_Unknown;
                                result = result && !band->GetColorTable();
                                if (result)
                                {
                                    const auto bandNodata = band->GetNoDataValue();                                    
                                    if (compose && bandNodata != noData)
                                        result = false;
                                    else
                                        noData = bandNodata;
                                }
                                if (result && compose && outRaster)
                                {
                                    auto pValues = reinterpret_cast<float*>(pData);
                                    std::fill(pValues, pValues + valueCount, static_cast<float>(noData));
                                }
                                result = result && band->RasterIO(GF_Read,
                                    dataOffset.x, dataOffset.y,
                                    dataSize.x, dataSize.y,
                                    pData + pShift, tileLength.x, tileLength.y,
                                    dataType, 0, side, &extraArg) == CE_None;
                                if (!result) break;
                                if (!procParameters)
                                    getMinMaxValues(pData, dataType, noData, valueCount, minValue, maxValue);
                                pData += bandSize;
                            }
                            if (result && tileSize > 1)
                            {
                                // Check values ​​in the center and corners to know that data is present
                                const auto halfSize = tileSize >> 1;
                                const auto middle = halfSize * halfSize * pixelSizeInBytes;
                                if (!compose)
                                {
                                    result = isDataPresent(pByteBuffer + middle, dataType, noData);
                                    result = result &&
                                        isDataPresent(pByteBuffer, dataType, noData);
                                    result = result &&
                                        isDataPresent(pByteBuffer + side - pixelSizeInBytes, dataType, noData);
                                    result = result &&
                                        isDataPresent(pByteBuffer + bandSize - pixelSizeInBytes, dataType, noData);
                                    result = result &&
                                        isDataPresent(pByteBuffer + bandSize - side, dataType, noData);
                                }
                                // Check value in the center of top overscaled tile
                                if (result && zoomShift > 0 && minZoom > MinZoomLevel)
                                {
                                    upperLeft = metersFrom31(upperLeftOverscaled.x, upperLeftOverscaled.y);
                                    lowerRight = metersFrom31(lowerRightOverscaled.x, lowerRightOverscaled.y);
                                    upperLeft.x = qMax((upperLeft.x - geoTransform[0]) / geoTransform[1], 0.0);
                                    upperLeft.y = qMax((upperLeft.y - geoTransform[3]) / geoTransform[5], 0.0);
                                    lowerRight.x = (lowerRight.x - geoTransform[0]) / geoTransform[1];
                                    lowerRight.y = (lowerRight.y - geoTransform[3]) / geoTransform[5];
                                    extraArg.dfXOff = (upperLeft.x + lowerRight.x) * 0.5;
                                    extraArg.dfYOff = (upperLeft.y + lowerRight.y) * 0.5;
                                    extraArg.dfXSize = 1.0;
                                    extraArg.dfYSize = 1.0;
                                    band = dataset->GetRasterBand(1);
                                    double centerValue = noData;
                                    result = band->RasterIO(GF_Read,
                                        std::floor(extraArg.dfXOff), std::floor(extraArg.dfYOff),
                                        1, 1, &centerValue, 1, 1, GDT_Float64, 0, 0, &extraArg) == CE_None;
                                    result = result && centerValue != noData;
                                }
                                if (!result)
                                    empty = true;
                            }
                            // Hillshade/slope raster processing
                            if (procParameters)
                            {
                                if (compose)
                                {
                                    if (result)
                                    {
                                        // Compensate height blur effect for much downsampled tiles
                                        const float scaleFactor =
                                            pow(1.5f, static_cast<float>(qMin(minZoom - zoom, 3))) - 0.2f;
                                        
                                        // Combine heightmap data
                                        mergeHeights(tileSize, scaleFactor, !atLeastOnePresent,
                                            compositeTile, reinterpret_cast<float*>(pByteBuffer));

                                        if (!atLeastOnePresent)
                                        {
                                            compositeOrigin = tileOrigin;
                                            compositeResolution = tileResolution;
                                            compositeSpecification = specification;
                                            compositeGCPCount = gcpCount;
                                            compositeGCPList = QByteArray(
                                                reinterpret_cast<const char*>(gcpList), sizeof(GDAL_GCP) * gcpCount);
                                            atLeastOnePresent = true;
                                        }
                                    }
                                    else
                                        incomplete = true;
                                }
                                else
                                {
                                    // Produce hillshade/slope raster from heightmap data
                                    result = result && destDataType == GDT_Byte && bandCount == 4 &&
                                        postProcess(pByteBuffer, *procParameters, tileSize, overlap, tileOrigin,
                                            tileResolution, gcpCount, gcpList, pBuffer);                                
                                }
                                delete[] pByteBuffer;
                                pByteBuffer = static_cast<char*>(pBuffer);
                                bandSize = (tileSize - overlap) * (tileSize - overlap) *
                                    (destDataType == GDT_Byte ? 1 : (destDataType == GDT_Int16 ? 2 : 4));
                            }
                            if (empty)
                                result = true;
                            // Put raster data in cache database file
                            if (result && !empty && !compose && cacheDatabase && cacheDatabase->isOpened())
                            {
                                const auto currentTime = QDateTime::currentMSecsSinceEpoch();
                                if (!procParameters && minValue != std::numeric_limits<float>::max()
                                    && maxValue != std::numeric_limits<float>::lowest())
                                {
                                    cacheDatabase->storeTileData(tileId, zoom, specification,
                                        QByteArray::fromRawData(pByteBuffer, bandCount * bandSize),
                                        currentTime, 0, minValue, maxValue);
                                }
                                else
                                {
                                    cacheDatabase->storeTileData(tileId, zoom, specification,
                                        QByteArray::fromRawData(pByteBuffer, bandCount * bandSize), currentTime);
                                }
                            }
                        }
                        GDALClose(dataset);
                    }
                    if (!compose && !empty)
                    {
                        if (result)
                            return GeoTiffCollection::CallResult::Completed;
                        else
                            return GeoTiffCollection::CallResult::Failed;
                    }
                }
            }
        }
    }

    // Build result composite tile and put it in cache
    if (compose && available && destDataType == GDT_Byte && bandCount == 4)
    {
        bool result = true;
        if (atLeastOnePresent)
        {
            // Produce hillshade/slope raster from heightmap data
            result = postProcess(compositeTile.constData(), *procParameters, tileSize, overlap, compositeOrigin,
                compositeResolution, compositeGCPCount, compositeGCPList.constData(), pBuffer);
        }
        else if (incomplete)
        {
            // No data due to failed data request
            return GeoTiffCollection::CallResult::Failed;
        }
        else
        {
            // Produce empty raster if no data was found
            memset(pBuffer, bandCount * bandSize, 0);
        }
        if (result && !incomplete && cacheDatabase && cacheDatabase->isOpened())
        {
            const auto currentTime = QDateTime::currentMSecsSinceEpoch();
            cacheDatabase->storeTileData(tileId, zoom, compositeSpecification,
                QByteArray::fromRawData(pByteBuffer, bandCount * bandSize), currentTime);
        }
        if (result)
            return GeoTiffCollection::CallResult::Completed;
    }

    return GeoTiffCollection::CallResult::Empty;
}

void OsmAnd::GeoTiffCollection_P::onDirectoryChanged(const QString& path)
{
    invalidateCollectedSources();
}

void OsmAnd::GeoTiffCollection_P::onFileChanged(const QString& path)
{
    invalidateCollectedSources();
}

bool OsmAnd::GeoTiffCollection_P::calculateHeights(
	const ZoomLevel zoom,
	const uint32_t tileSize,
	const QList<PointI>& points31,
	QList<float>& outHeights) const
{
    std::shared_ptr<IMapElevationDataProvider::Data> lastData;
    for (const auto& point31 : constOf(points31))
    {
        PointF offsetInTileN;
        TileId tileId = Utilities::getTileId(point31, zoom, &offsetInTileN);
        if (lastData && lastData->tileId == tileId)
        {
            float elevationInMeters = 0.0f;
            if (lastData->getValue(offsetInTileN, elevationInMeters))
                outHeights.append(elevationInMeters);
            else
                return false;
        }
        else
        {
            float minValue, maxValue;
            const auto pBuffer = new float[tileSize * tileSize];
            const auto result =
                getGeoTiffData(tileId, zoom, tileSize, 3, 1, false, minValue, maxValue, pBuffer, nullptr);
            if (result == GeoTiffCollection::CallResult::Completed)
            {
                auto data = std::make_shared<IMapElevationDataProvider::Data>(
                    tileId,
                      zoom,
                      sizeof(float)*tileSize,
                      tileSize,
                      minValue,
                      maxValue,
                      pBuffer);

                float elevationInMeters = 0.0f;
                if (data->getValue(offsetInTileN, elevationInMeters))
                    outHeights.append(elevationInMeters);
                else
                    return false;

                lastData = data;
            }
            else
            {
                delete[] pBuffer;
                return false;
            }
        }
    }
    return true;
}

