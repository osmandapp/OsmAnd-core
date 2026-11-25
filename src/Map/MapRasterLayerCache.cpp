#include "MapRasterLayerCache.h"
#include "MapPrimitivesCache.h"
#include "MapPresentationEnvironment.h"
#include "MapStyleConstantValue.h"
#include "MapStyleValueDefinition.h"
#include "ObfMapObjectsProvider.h"
#include "IObfsCollection.h"
#include "ObfFile.h"
#include "ObfInfo.h"
#include "Utilities.h"
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QtEndian>
#include <SkData.h>
#include <SkPixmap.h>
#include <SkStream.h>
#include <webp/decode.h>
#include <include/encode/SkWebpEncoder.h>
#include <sqlite3.h>
#include "ICoreResourcesProvider.h"
#include "IMapStyle.h"
#include <QMap>
#include <QSet>
#include <map>

#ifndef OSMAND_RASTER_CACHE_WRITE_THREADS
#define OSMAND_RASTER_CACHE_WRITE_THREADS 16
#endif

namespace
{
    // This describes immutable style/resource content; mutable environment settings
    // remain part of the individual tile key.
    class StyleFingerprint
    {
        using Style = OsmAnd::IMapStyle;
        using DataType = OsmAnd::MapStyleValueDataType;
        const Style& _style;
        QHash<const Style::IRuleNode*, QByteArray> _nodes;
        QSet<const Style::IRuleNode*> _activeNodes;
        int _nodeCount = 0;

        static QByteArray digest(const QByteArray& bytes)
        {
            return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
        }

        QString string(Style::StringId id)
        {
            const auto result = _style.getStringById(id);
            if (id != Style::EmptyStringId && result.isNull())
                valid = false;
            return result.isEmpty() ? QStringLiteral("") : result;
        }

        void constant(QDataStream& stream, DataType type,
            const OsmAnd::MapStyleConstantValue& value, bool resource = false)
        {
            stream << qint32(type) << value.isComplex;
            if (value.isComplex)
            {
                if (type == DataType::Float)
                    stream << value.asComplex.asFloat.pt << value.asComplex.asFloat.px;
                else if (type == DataType::Integer)
                    stream << value.asComplex.asInt.pt << value.asComplex.asInt.px;
                else
                    valid = false;
            }
            else if (type == DataType::String)
            {
                const auto text = string(value.asSimple.asUInt);
                stream << text;
                if (resource && !text.isEmpty())
                    resourceNames.insert(text);
            }
            else if (type == DataType::Float)
                stream << value.asSimple.asFloat;
            else if (type == DataType::Integer || type == DataType::Boolean || type == DataType::Color)
                stream << value.asSimple.asUInt;
            else
                valid = false;
        }

        void symbolClass(QDataStream& stream, const std::shared_ptr<const Style::ISymbolClass>& value)
        {
            if (!value)
            {
                valid = false;
                return;
            }
            stream << string(value->getNameId()) << value->getDefaultSetting()
                << value->getTitle() << value->getDescription() << value->getCategory()
                << value->getLegendObject() << value->getInnerLegendObject()
                << value->getInnerTitle() << value->getInnerDescription()
                << value->getInnerCategory() << value->getInnerNames();
        }

        QByteArray node(const std::shared_ptr<const Style::IRuleNode>& value, int depth)
        {
            if (!valid || !value || depth > 256 || _activeNodes.contains(value.get()))
            {
                valid = false;
                return {};
            }
            const auto cached = _nodes.constFind(value.get());
            if (cached != _nodes.cend())
                return *cached;
            if (++_nodeCount > 1000000)
            {
                valid = false;
                return {};
            }
            _activeNodes.insert(value.get());
            QByteArray bytes;
            QDataStream stream(&bytes, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_0);
            stream << value->getIsSwitch();
            QMap<QString, Style::ValueDefinitionId> definitions;
            const auto& values = value->getValuesRef();
            for (auto it = values.cbegin(); it != values.cend(); ++it)
            {
                const auto definition = _style.getValueDefinitionById(it.key());
                if (!definition || definition->name.isEmpty() || definitions.contains(definition->name))
                {
                    valid = false;
                    return {};
                }
                definitions.insert(definition->name, it.key());
            }
            stream << quint32(definitions.size());
            for (auto it = definitions.cbegin(); it != definitions.cend(); ++it)
            {
                const auto definition = _style.getValueDefinitionById(it.value());
                const auto& v = values.constFind(it.value()).value();
                stream << it.key() << qint32(definition->valueClass) << definition->isComplex << v.isDynamic;
                const bool resource = it.key() == QLatin1String("shader")
                    || it.key() == QLatin1String("pathIcon") || it.key() == QLatin1String("attrStringValue");
                constant(stream, definition->dataType, v.asConstantValue, resource);
                if (!v.isDynamic)
                    continue;
                const auto& dynamic = v.asDynamicValue;
                stream << bool(dynamic.attribute) << bool(dynamic.symbolClasses) << bool(dynamic.symbolClassTemplates);
                if (!dynamic.attribute && !dynamic.symbolClasses && !dynamic.symbolClassTemplates)
                    valid = false;
                if (dynamic.attribute)
                    stream << string(dynamic.attribute->getNameId())
                        << node(dynamic.attribute->getRootNodeRef(), depth + 1);
                if (dynamic.symbolClasses)
                {
                    // Class membership is an OR test; wildcard expansion may come
                    // from a QHash and must not retain process-dependent ordering.
                    QList<QByteArray> classes;
                    for (const auto& item : *dynamic.symbolClasses)
                    {
                        QByteArray bytes;
                        QDataStream member(&bytes, QIODevice::WriteOnly);
                        member.setVersion(QDataStream::Qt_5_0);
                        symbolClass(member, item);
                        classes.append(bytes);
                    }
                    std::sort(classes.begin(), classes.end());
                    stream << classes;
                }
                if (dynamic.symbolClassTemplates)
                {
                    QList<QString> templates;
                    for (const auto id : *dynamic.symbolClassTemplates)
                        templates.append(string(id));
                    std::sort(templates.begin(), templates.end());
                    stream << templates;
                }
            }
            const auto writeChildren = [&](const QList<std::shared_ptr<const Style::IRuleNode>>& children)
            {
                stream << quint32(children.size());
                for (const auto& child : children)
                    stream << node(child, depth + 1);
            };
            writeChildren(value->getOneOfConditionalSubnodesRef());
            writeChildren(value->getApplySubnodesRef());
            _activeNodes.remove(value.get());
            if (stream.status() != QDataStream::Ok)
                valid = false;
            const auto result = digest(bytes);
            _nodes.insert(value.get(), result);
            return result;
        }

        template<class T> QMap<QString, std::shared_ptr<const T>> named(
            const QList<std::shared_ptr<const T>>& values)
        {
            QMap<QString, std::shared_ptr<const T>> result;
            for (const auto& value : values)
            {
                if (!value)
                {
                    valid = false;
                    break;
                }
                const auto name = string(value->getNameId());
                if (name.isEmpty() || result.contains(name))
                {
                    valid = false;
                    break;
                }
                result.insert(name, value);
            }
            return result;
        }

    public:
        bool valid = true;
        QSet<QString> resourceNames;

        explicit StyleFingerprint(const Style& style) : _style(style) {}

        QByteArray calculate()
        {
            QByteArray bytes;
            QDataStream stream(&bytes, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_0);
            stream << quint32(1); // Canonical style/resource fingerprint format.
            QMap<QString, std::shared_ptr<const OsmAnd::MapStyleValueDefinition>> definitions;
            for (const auto& definition : _style.getValueDefinitions())
            {
                if (!definition || definition->name.isEmpty() || definitions.contains(definition->name))
                    return {};
                definitions.insert(definition->name, definition);
            }
            stream << quint32(definitions.size());
            for (const auto& definition : definitions)
                stream << definition->name << qint32(definition->valueClass)
                    << qint32(definition->dataType) << definition->isComplex;
            const auto parameters = named(_style.getParameters());
            stream << quint32(parameters.size());
            for (auto it = parameters.cbegin(); it != parameters.cend(); ++it)
            {
                const auto& parameter = it.value();
                stream << it.key() << qint32(parameter->getDataType()) << parameter->getTitle()
                    << parameter->getDescription() << parameter->getCategory()
                    << parameter->getDefaultValueDescription();
                const auto values = parameter->getPossibleValues();
                stream << quint32(values.size());
                for (const auto& value : values)
                    constant(stream, parameter->getDataType(), value);
            }
            const auto classes = named(_style.getSymbolClasses());
            stream << quint32(classes.size());
            for (const auto& value : classes)
                symbolClass(stream, value);
            const auto attributes = named(_style.getAttributes());
            stream << quint32(attributes.size());
            for (auto it = attributes.cbegin(); it != attributes.cend(); ++it)
                stream << it.key() << node(it.value()->getRootNodeRef(), 0);
            stream << quint32(OsmAnd::MapStyleRulesetTypesCount);
            for (unsigned int i = 0; i < OsmAnd::MapStyleRulesetTypesCount; ++i)
            {
                const auto type = static_cast<OsmAnd::MapStyleRulesetType>(i);
                const auto ruleset = _style.getRuleset(type);
                QMap<QPair<QString, QString>, std::shared_ptr<const Style::IRule>> rules;
                for (auto it = ruleset.cbegin(); it != ruleset.cend(); ++it)
                {
                    const auto key = qMakePair(string(it.key().tagId), string(it.key().valueId));
                    if (!it.value() || it.value()->getRulesetType() != type || rules.contains(key))
                        return {};
                    rules.insert(key, it.value());
                }
                stream << qint32(type) << quint32(rules.size());
                for (auto it = rules.cbegin(); it != rules.cend(); ++it)
                    stream << it.key().first << it.key().second << node(it.value()->getRootNodeRef(), 0);
            }
            return valid && stream.status() == QDataStream::Ok ? digest(bytes) : QByteArray();
        }
    };

    QByteArray calculateEnvironmentKey(const OsmAnd::MapPresentationEnvironment& environment)
    {
        if (!environment.mapStyle)
            return {};
        StyleFingerprint fingerprint(*environment.mapStyle);
        const auto styleKey = fingerprint.calculate();
        if (styleKey.isEmpty())
            return {};
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_0);
        stream << styleKey;
        auto names = fingerprint.resourceNames.values();
        std::sort(names.begin(), names.end());
        stream << quint32(names.size());
        const auto& external = environment.externalResourcesProvider;
        const auto& core = OsmAnd::getCoreResourcesProvider();
        const auto density = environment.displayDensityFactor;
        for (const auto& name : names)
        {
            stream << name;
            // Include both namespaces and color variants: adding a higher-priority
            // resource changes rendering even when the old resource is unchanged.
            for (const auto& prefix : { QStringLiteral("map/shaders_and_shields/"), QStringLiteral("map/icons/") })
            for (const auto& color : { QStringLiteral("c_"), QStringLiteral("") })
            {
                const auto path = prefix + color + name + QStringLiteral(".svg");
                const bool externalDensity = external && external->containsResource(path, density);
                const bool externalBase = external && external->containsResource(path);
                const bool coreDensity = core && core->containsResource(path, density);
                const bool coreBase = core && core->containsResource(path);
                stream << externalDensity << externalBase << coreDensity << coreBase;
                // containsResource is also the renderer's namespace-selection test.
                // Avoid asking providers for absent paths, which may log warnings.
                if (!externalDensity && !externalBase && !coreDensity && !coreBase)
                    continue;
                bool ok = false;
                QByteArray data;
                if (external)
                    data = externalDensity
                        ? external->getResource(path, density, &ok)
                        : external->getResource(path, &ok);
                stream << ok;
                if (!ok && core)
                    data = coreDensity ? core->getResource(path, density, &ok) : core->getResource(path, &ok);
                if (!ok)
                    return {};
                stream << QCryptographicHash::hash(data, QCryptographicHash::Sha256);
            }
        }
        return stream.status() == QDataStream::Ok
            ? QCryptographicHash::hash(bytes, QCryptographicHash::Sha256) : QByteArray();
    }

    QByteArray environmentKey(const std::shared_ptr<const OsmAnd::MapPresentationEnvironment>& environment)
    {
        // Match IconsProvider's lifetime: unchanged environments retain their asset snapshot.
        // Weak ownership avoids keeping old environments and their resources alive.
        using Environment = std::weak_ptr<const OsmAnd::MapPresentationEnvironment>;
        static QMutex mutex;
        static std::map<Environment, QByteArray, std::owner_less<Environment>> fingerprints;
        QMutexLocker lock(&mutex);
        for (auto it = fingerprints.begin(); it != fingerprints.end();)
            it = it->first.expired() ? fingerprints.erase(it) : std::next(it);
        const Environment owner(environment);
        const auto existing = fingerprints.find(owner);
        if (existing != fingerprints.end())
            return existing->second;
        lock.unlock();
        const auto key = calculateEnvironmentKey(*environment);
        if (key.isEmpty())
            return {}; // Allow a later setup to retry a transient resource-read failure.
        lock.relock();
        return fingerprints.emplace(owner, key).first->second;
    }

    constexpr quint32 Version = 1;
    constexpr int MaxRecordSize = 64 * 1024 * 1024;

    std::shared_ptr<QReadWriteLock> databaseLock(const QString& filename)
    {
        const QFileInfo file(filename);
        auto path = file.canonicalFilePath();
        if (path.isEmpty())
            path = file.dir().canonicalPath() + QLatin1Char('/') + file.fileName();

        static QMutex mutex;
        static QHash<QString, std::weak_ptr<QReadWriteLock>> locks;
        QMutexLocker locker(&mutex);
        for (auto it = locks.begin(); it != locks.end();)
            it = it.value().expired() ? locks.erase(it) : std::next(it);

        auto& weak = locks[path];
        auto lock = weak.lock();
        if (!lock)
        {
            lock = std::make_shared<QReadWriteLock>();
            weak = lock;
        }
        return lock;
    }

    qint64 specification(const QByteArray& key)
    {
        return qMax<qint64>(1, qint64(qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(key.constData()))
            & 0x7fffffffffffffffULL));
    }

    int detailZoom(const OsmAnd::IMapTiledDataProvider::Request& request)
    {
        return request.detailedZoom == OsmAnd::InvalidZoomLevel ? request.zoom : request.detailedZoom;
    }

    sk_sp<SkData> encodeWebp(const sk_sp<SkImage>& image)
    {
        const auto raster = image->makeRasterImage();
        SkPixmap pixels;
        if (!raster || !raster->peekPixels(&pixels))
            return nullptr;
        SkWebpEncoder::Options options;
        options.fCompression = SkWebpEncoder::Compression::kLossless;
        options.fQuality = 75.0f; // Compression effort in lossless mode.
        SkDynamicMemoryWStream stream;
        return SkWebpEncoder::Encode(&stream, pixels, options) ? stream.detachAsData() : nullptr;
    }

    sk_sp<SkImage> decodeWebp(const QByteArray& bytes, unsigned int tileSize)
    {
        const auto info = SkImageInfo::Make(tileSize, tileSize, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        const auto src = reinterpret_cast<const uint8_t*>(bytes.constData());
        WebPBitstreamFeatures features{};
        if (!bytes.startsWith("RIFF") || WebPGetFeatures(src, bytes.size(), &features) != VP8_STATUS_OK
            || features.format != 2 || features.has_animation
            || features.width != info.width() || features.height != info.height())
            return nullptr;
        const auto pixels = SkData::MakeUninitialized(info.computeMinByteSize());
        if (!pixels || !WebPDecodeRGBAInto(src, bytes.size(), static_cast<uint8_t*>(pixels->writable_data()),
            pixels->size(), int(info.minRowBytes())))
            return nullptr;
        // libwebp's direct premultiplied output can truncate. Skia rounds to nearest.
        auto* rgba = static_cast<uint8_t*>(pixels->writable_data());
        for (size_t i = 0; i < pixels->size(); i += 4)
        {
            const auto alpha = rgba[i + 3];
            if (alpha != 255)
                for (int c = 0; c < 3; ++c)
                    rgba[i + c] = (rgba[i + c] * alpha + 127) / 255;
        }
        return SkImage::MakeRasterData(info, pixels, info.minRowBytes());
    }

    // Inspect the owner before TileSqliteDatabase::open() creates any tables.
    bool prepareCacheFile(const QString& filename)
    {
        const QByteArray format("webp-raster-primitives-v1");
        sqlite3* handle = nullptr;
        const auto rc = sqlite3_open_v2(filename.toUtf8().constData(), &handle,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        const std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(handle, sqlite3_close);
        if (rc != SQLITE_OK)
            return false;
        QByteArray stored;
        const auto readValue = [](void* value, int, char** columns, char**) {
            *static_cast<QByteArray*>(value) = columns[0];
            return 0;
        };
        if (sqlite3_exec(handle, "SELECT raster_cache_format FROM info LIMIT 1",
            readValue, &stored, nullptr) == SQLITE_OK)
            return stored == format;

        QByteArray hasSchema;
        if (sqlite3_exec(handle, "SELECT 1 FROM sqlite_master LIMIT 1", readValue, &hasSchema, nullptr) != SQLITE_OK
            || !hasSchema.isEmpty())
            return false;
        // Publish ownership atomically before creating the tile schema. An interrupted setup can retry.
        const auto create = "CREATE TABLE info AS SELECT '" + format + "' AS raster_cache_format";
        return sqlite3_exec(handle, create.constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
    }

}

OsmAnd::MapRasterLayerCache::MapRasterLayerCache(
    const std::shared_ptr<MapPrimitivesProvider>& provider, const QString& filename,
    bool fillBackground, uint64_t maxBytes, ZoomLevel minZoom, ZoomLevel maxZoom)
    : _provider(provider)
    , _environment(provider->primitiviser->environment)
    , _obfProvider(std::dynamic_pointer_cast<ObfMapObjectsProvider>(provider->mapObjectsProvider))
    , _tileSize(provider->tileSize)
    , _mode(provider->mode)
    , _environmentKey(environmentKey(_environment))
    , _fillBackground(fillBackground)
    , _maxBytes(maxBytes)
    , _minZoom(minZoom)
    , _maxZoom(maxZoom)
    , _database(filename, false)
{
    _writePool.setMaxThreadCount(OSMAND_RASTER_CACHE_WRITE_THREADS);
    if (!_obfProvider || _environmentKey.isEmpty() || _mode != MapPrimitivesProvider::Mode::WithSurface
        || _tileSize == 0 || _tileSize > 4096 || maxBytes < 64 * 1024
        || minZoom < MinZoomLevel || minZoom > maxZoom || maxZoom > MaxZoomLevel
        || !QDir().mkpath(QFileInfo(filename).absolutePath()))
        return;

    // Replacement providers may open the same file before the old layer is detached.
    _databaseLock = databaseLock(filename);
    QWriteLocker lock(_databaseLock.get());
    QLockFile setupLock(QFileInfo(filename).absoluteFilePath() + QStringLiteral(".lock"));
    if (!setupLock.tryLock(5000) || !prepareCacheFile(filename) || !_database.open(true))
        return;

    TileSqliteDatabase::Meta meta;
    if (!_database.hasSpecificationColumn() || !_database.obtainMeta(meta))
    {
        _database.close(false, false);
        return;
    }
    meta.setMinZoom(minZoom);
    meta.setMaxZoom(maxZoom);
    meta.setTileNumbering(QStringLiteral("OSM"));
    meta.setSpecificated(QStringLiteral("yes"));
    meta.setTileSize(_tileSize);
    if (!_database.storeMeta(meta) || !_database.enableTileTimeSupport()
        || !_database.enableTileTimestampSupport() || !_database.enableRasterTileSupport()
        || !_database.trimToSize(_maxBytes - _maxBytes / 10, (_maxBytes / 10) * 6))
        _database.close(false, false);
}

OsmAnd::MapRasterLayerCache::~MapRasterLayerCache()
{
    _writePool.waitForDone();
    // Raster cache metadata has already been persisted. Avoid issuing SQL here
    // after the provider has been detached and the file handed over.
    _database.close(false, false);
}

bool OsmAnd::MapRasterLayerCache::isOpened() const
{
    return _database.isOpened();
}

void OsmAnd::MapRasterLayerCache::setSuspended(bool suspended)
{
    // Finish accepted writes before handing off the file. Workers need the database lock to commit.
    QWriteLocker admission(&_writeAdmissionLock);
    if (suspended)
        _writePool.waitForDone();
    QWriteLocker lock(_databaseLock.get());
    if (_suspended != suspended)
    {
        _suspended = suspended;
        ++_generation;
    }
}

bool OsmAnd::MapRasterLayerCache::isCurrentKey(const QByteArray& key) const
{
    return !_suspended && qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(key.constData() + 32)) == _generation;
}

QByteArray OsmAnd::MapRasterLayerCache::key(const IMapTiledDataProvider::Request& request) const
{
    if (!isOpened() || request.zoom < _minZoom || request.zoom > _maxZoom
        || detailZoom(request) < MinZoomLevel || detailZoom(request) > MaxZoomLevel)
        return {};

    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_0);
    stream << Version << _environmentKey << quint32(_tileSize) << qint32(_mode) << qint32(_obfProvider->mode)
        << _fillBackground << request.tileId.x << request.tileId.y << qint32(request.zoom)
        << qint32(detailZoom(request)) << _environment->displayDensityFactor
        << _environment->mapScaleFactor << _environment->symbolsScaleFactor
        << _environment->getLocaleLanguageId() << qint32(_environment->getLanguagePreference());
    auto disabled = _environment->disabledAttributes.values();
    std::sort(disabled.begin(), disabled.end());
    stream << disabled;
    // Names and actual values are stable across runs; style definition/string IDs are not.
    QMap<QString, QByteArray> settings;
    const auto style = _environment->mapStyle;
    const auto values = _environment->getSettings();
    for (auto it = values.cbegin(); it != values.cend(); ++it)
    {
        const auto definition = style->getValueDefinitionById(it.key());
        if (!definition)
            return {};
        QByteArray value;
        QDataStream s(&value, QIODevice::WriteOnly);
        s.setVersion(QDataStream::Qt_5_0);
        const auto& v = it.value();
        s << qint32(definition->dataType) << v.isComplex;
        if (v.isComplex)
        {
            if (definition->dataType == MapStyleValueDataType::Float)
                s << v.asComplex.asFloat.pt << v.asComplex.asFloat.px;
            else
                s << v.asComplex.asInt.pt << v.asComplex.asInt.px;
        }
        else if (definition->dataType == MapStyleValueDataType::String)
            s << style->getStringById(v.asSimple.asUInt);
        else if (definition->dataType == MapStyleValueDataType::Float)
            s << v.asSimple.asFloat;
        else
            s << v.asSimple.asUInt;
        settings.insert(definition->name, value);
    }
    stream << settings;

    // Only intersecting sources participate: a regional map update leaves other regions usable.
    // Include live updates too rather than silently serving an older map after an update.
    const auto bbox = Utilities::tileBoundingBox31(request.tileId, request.zoom);
    QMap<QString, QByteArray> sources;
    for (const auto& file : _obfProvider->obfsCollection->getObfFiles())
    {
        if (!file->obfInfo)
            continue;
        if (!file->obfInfo->isBasemap && !file->obfInfo->isBasemapWithCoastlines
            && !file->obfInfo->containsDataFor(&bbox, MinZoomLevel, MaxZoomLevel,
            ObfDataTypesMask().set(ObfDataType::Map).set(ObfDataType::Routing)))
            continue;
        const QFileInfo info(file->filePath);
        QByteArray source;
        QDataStream s(&source, QIODevice::WriteOnly);
        s.setVersion(QDataStream::Qt_5_0);
        s << quint64(file->fileSize) << quint64(file->obfInfo->creationTimestamp)
            << info.size() << info.lastModified().toMSecsSinceEpoch();
        sources.insert(file->filePath, source);
    }
    stream << sources;
    auto result = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    // This suffix is an in-process cancellation token, not part of the persistent identity.
    QReadLocker lock(_databaseLock.get());
    if (_suspended)
        return {};
    QDataStream suffix(&result, QIODevice::Append);
    suffix << _generation;
    return result;
}

template<class Decode>
bool OsmAnd::MapRasterLayerCache::readPayload(const IMapTiledDataProvider::Request& request,
    const QByteArray& cacheKey, TileSqliteDatabase::RasterPayload payload, Decode decode)
{
    if (cacheKey.size() != 40 || !isOpened())
        return false;
    QByteArray bytes;
    int64_t lastAccess = 0;
    {
        QReadLocker lock(_databaseLock.get());
        if (!isCurrentKey(cacheKey))
            return false;
        if (!_database.retrieveRasterPayload(request.tileId, request.zoom, specification(cacheKey), payload,
            ZoomLevel(detailZoom(request)), cacheKey.left(32), MaxRecordSize - 32, bytes, lastAccess))
            return false;
    }
    if (!decode(bytes) || cacheKey != key(request))
        return false;

    // Refresh recency at most once per minute for this tile.
    const auto now = QDateTime::currentMSecsSinceEpoch();
    if (lastAccess <= now - 60000)
    {
        QWriteLocker lock(_databaseLock.get());
        if (isCurrentKey(cacheKey))
            _database.updateTileTimestamp(request.tileId, request.zoom, specification(cacheKey), now);
    }
    return true;
}

bool OsmAnd::MapRasterLayerCache::readImage(const IMapTiledDataProvider::Request& request,
    const QByteArray& cacheKey, sk_sp<SkImage>& image)
{
    sk_sp<SkImage> decoded;
    if (!readPayload(request, cacheKey, TileSqliteDatabase::RasterPayload::Image, [&](const QByteArray& bytes)
        {
            decoded = decodeWebp(bytes, _tileSize);
            return bool(decoded);
        }))
        return false;
    image = std::move(decoded);
    return true;
}

bool OsmAnd::MapRasterLayerCache::readPrimitives(const IMapTiledDataProvider::Request& request,
    const QByteArray& cacheKey, std::shared_ptr<IMapObjectsProvider::Data>& objects,
    std::shared_ptr<MapPrimitiviser::PrimitivisedObjects>& primitives)
{
    const auto provider = _provider.lock();
    std::shared_ptr<IMapObjectsProvider::Data> decodedObjects;
    std::shared_ptr<MapPrimitiviser::PrimitivisedObjects> decodedPrimitives;
    if (!provider || !readPayload(request, cacheKey, TileSqliteDatabase::RasterPayload::Primitives,
        [&](const QByteArray& bytes) {
            return MapPrimitivesCache::deserialize(bytes, *provider, request, decodedObjects, decodedPrimitives);
        }))
        return false;
    objects = std::move(decodedObjects);
    primitives = std::move(decodedPrimitives);
    return true;
}

void OsmAnd::MapRasterLayerCache::write(const IMapTiledDataProvider::Request& request,
    const QByteArray& cacheKey, const sk_sp<SkImage>& image,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& data)
{
    QReadLocker admission(&_writeAdmissionLock);
    if (_suspended || OSMAND_RASTER_CACHE_WRITE_THREADS <= 0)
        return;

    // A busy pool skips this tile. Rendering can continue as soon as submission returns.
    _writePool.tryStart(
        [this, tileId = request.tileId, zoom = request.zoom, detail = request.detailedZoom, cacheKey, image, data] {
            IMapTiledDataProvider::Request tile;
            tile.tileId = tileId;
            tile.zoom = zoom;
            tile.detailedZoom = detail;
            writeNow(tile, cacheKey, image, data);
        });
}

void OsmAnd::MapRasterLayerCache::writeNow(const IMapTiledDataProvider::Request& request,
    const QByteArray& cacheKey, const sk_sp<SkImage>& image,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& data)
{
    if (cacheKey.size() != 40 || !image || !data || image->width() != int(_tileSize)
        || image->height() != int(_tileSize) || cacheKey != key(request))
        return;

    const auto created = QDateTime::currentMSecsSinceEpoch();
    TileSqliteDatabase::RasterTileData raster;
    raster.primitives = MapPrimitivesCache::serialize(*data);
    raster.detailedZoom = ZoomLevel(detailZoom(request));
    raster.cacheKey = cacheKey.left(32);
    if (raster.primitives.isEmpty())
        return;
    const auto encoded = encodeWebp(image);
    if (!encoded || encoded->isEmpty())
        return;
    const auto size = quint64(encoded->size()) + raster.primitives.size() + raster.cacheKey.size();
    if (size > MaxRecordSize || size > _maxBytes / 4 || cacheKey != key(request))
        return;

    // Encoding runs concurrently. Serialize the commit against cache invalidation.
    QWriteLocker lock(_databaseLock.get());
    if (!isCurrentKey(cacheKey))
        return;
    if (_database.storeTileData(request.tileId, request.zoom, specification(cacheKey),
        QByteArray::fromRawData(static_cast<const char*>(encoded->data()), int(encoded->size())),
        created, created, 0, 0, &raster, false))
    {
        _database.trimToSize(_maxBytes - _maxBytes / 10, (_maxBytes / 10) * 6, _maxBytes / 10);
    }
}

bool OsmAnd::MapRasterLayerCache::clear()
{
    QWriteLocker lock(_databaseLock.get());
    ++_generation;
    return _database.removeTilesData() && _database.compact();
}

bool OsmAnd::MapRasterLayerCache::removeOlderTiles(int64_t time)
{
    QWriteLocker lock(_databaseLock.get());
    // Retained tiles and in-flight writes must not restore records removed by this operation.
    ++_generation;
    return _database.removeOlderTilesData(time);
}
