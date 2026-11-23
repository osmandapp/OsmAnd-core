#include "MapPrimitivesCache.h"

#include <QBitArray>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QMetaType>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <typeinfo>

#include "ignore_warnings_on_external_includes.h"
#include <zlib.h>
#include "restore_internal_warnings.h"

#include "BinaryMapObject.h"
#include "IObfsCollection.h"
#include "MapPresentationEnvironment.h"
#include "MapStyleValueDefinition.h"
#include "ObfFile.h"
#include "ObfInfo.h"
#include "ObfMapObjectsProvider.h"
#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionInfo.h"
#include "Road.h"

namespace
{
    using namespace OsmAnd;
    using MP = MapPrimitiviser;
    constexpr quint32 Version = 1;
    // Version, uncompressed length, and SHA-256 of the uncompressed graph.
    constexpr int HeaderSize = 2 * sizeof(quint32) + 32;
    constexpr quint32 MaxBytes = 128u * 1024 * 1024;
    constexpr quint32 MaxItems = 1u << 20;
    struct Invalid {};
    void require(bool condition) { if (!condition) throw Invalid(); }

    void configure(QDataStream& s)
    {
        s.setVersion(QDataStream::Qt_5_0);
        s.setByteOrder(QDataStream::LittleEndian);
    }

    struct Writer
    {
        QByteArray bytes;
        QDataStream s;
        Writer() : s(&bytes, QIODevice::WriteOnly) { configure(s); }
        void count(int n)
        {
            require(n >= 0 && quint32(n) <= MaxItems && quint32(bytes.size()) <= MaxBytes);
            s << quint32(n);
        }
        void str(const QString& v)
        {
            require(v.size() <= int(MaxItems)
                && quint64(bytes.size()) + quint64(v.size()) * 2 + 4 <= MaxBytes);
            s << v;
        }
        void f32(float v) { quint32 b; memcpy(&b, &v, sizeof b); s << b; }
        void f64(double v) { quint64 b; memcpy(&b, &v, sizeof b); s << b; }
        void point(const PointI& p) { s << qint32(p.x) << qint32(p.y); }
        void area(const AreaI& a)
        {
            s << qint32(a.top()) << qint32(a.left()) << qint32(a.bottom()) << qint32(a.right());
        }
        void points(const QVector<PointI>& v) { count(v.size()); for (const auto& p : v) point(p); }
        template<class C> void ids(const C& v) { count(v.size()); for (auto n : v) s << quint32(n); }
        template<class C> void strings(const C& v) { count(v.size()); for (const auto& t : v) str(t); }
        void variant(const QVariant& v)
        {
            const auto type = v.userType();
            switch (type)
            {
                case QMetaType::Bool: s << quint8(1) << v.toBool(); break;
                case QMetaType::Int: s << quint8(2) << qint32(v.toInt()); break;
                case QMetaType::UInt: s << quint8(3) << quint32(v.toUInt()); break;
                case QMetaType::LongLong: s << quint8(4) << qint64(v.toLongLong()); break;
                case QMetaType::ULongLong: s << quint8(5) << quint64(v.toULongLong()); break;
                case QMetaType::Double: s << quint8(6); f64(v.toDouble()); break;
                case QMetaType::Float: s << quint8(7); f32(v.toFloat()); break;
                case QMetaType::QString: s << quint8(8); str(v.toString()); break;
                default: throw Invalid();
            }
        }
    };

    struct Reader
    {
        QBuffer buffer;
        QDataStream s;
        quint64 allocationBudget = quint64(MaxBytes) * 4;
        explicit Reader(const QByteArray& bytes) : s(&buffer)
        {
            buffer.setData(bytes);
            require(buffer.open(QIODevice::ReadOnly));
            configure(s);
        }
        template<class T> T scalar() { T v{}; s >> v; require(s.status() == QDataStream::Ok); return v; }
        quint8 u8() { return scalar<quint8>(); }
        quint32 u32() { return scalar<quint32>(); }
        qint32 i32() { return scalar<qint32>(); }
        quint64 u64() { return scalar<quint64>(); }
        qint64 i64() { return scalar<qint64>(); }
        bool boolean() { const auto v = u8(); require(v <= 1); return v != 0; }
        float f32() { const auto b = u32(); float v; memcpy(&v, &b, sizeof v); return v; }
        double f64() { const auto b = u64(); double v; memcpy(&v, &b, sizeof v); return v; }
        void account(quint64 size) { require(size <= allocationBudget); allocationBudget -= size; }
        quint32 count(quint32 minimumBytes = 1)
        {
            const auto n = u32();
            require(n <= MaxItems && quint64(n) * minimumBytes <= quint64(buffer.bytesAvailable()));
            account(quint64(n) * 64);
            return n;
        }
        QString str()
        {
            // QDataStream's container/string operators otherwise allocate before
            // checking the input size. Validate the QString byte length first.
            const auto pos = buffer.pos();
            const auto n = u32();
            require(n == 0xffffffffu || (n % 2 == 0 && n <= MaxItems * 2 && n <= quint64(buffer.bytesAvailable())));
            if (n != 0xffffffffu) account(n);
            require(buffer.seek(pos));
            QString v;
            s >> v;
            require(s.status() == QDataStream::Ok);
            return v;
        }
        PointI point() { const auto x = i32(); const auto y = i32(); return PointI(x, y); }
        AreaI area()
        {
            AreaI a;
            a.top() = i32(); a.left() = i32(); a.bottom() = i32(); a.right() = i32();
            return a;
        }
        QVector<PointI> points()
        {
            const auto n = count(8);
            QVector<PointI> v; v.reserve(n);
            for (quint32 i = 0; i < n; ++i) v.push_back(point());
            return v;
        }
        template<class C> C ids()
        {
            const auto n = count(4);
            C v;
            for (quint32 i = 0; i < n; ++i) v.push_back(u32());
            return v;
        }
        QList<QString> strings()
        {
            const auto n = count(4);
            QList<QString> v;
            for (quint32 i = 0; i < n; ++i) v.push_back(str());
            return v;
        }
        QVariant variant()
        {
            switch (u8())
            {
                case 1: return QVariant(boolean());
                case 2: return QVariant(i32());
                case 3: return QVariant(u32());
                case 4: return QVariant(i64());
                case 5: return QVariant(u64());
                case 6: return QVariant(f64());
                case 7: return QVariant::fromValue(f32());
                case 8: return QVariant(str());
                default: throw Invalid();
            }
        }
        template<class C> typename C::value_type ref(const C& table)
        {
            const auto i = u32(); require(i < quint32(table.size())); return table[int(i)];
        }
        bool atEnd() const { return buffer.atEnd() && s.status() == QDataStream::Ok; }
    };

    int objectKind(const MapObject& object)
    {
        if (typeid(object) == typeid(BinaryMapObject)) return 1;
        if (typeid(object) == typeid(Road)) return 2;
        if (typeid(object) == typeid(MP::CoastlineMapObject)) return 3;
        if (typeid(object) == typeid(MP::SurfaceMapObject)) return 4;
        require(typeid(object) == typeid(MapObject));
        return 0;
    }

    int mappingKind(const MapObject::AttributeMapping& mapping)
    {
        if (typeid(mapping) == typeid(ObfMapSectionAttributeMapping)) return 1;
        if (typeid(mapping) == typeid(ObfRoutingSectionAttributeMapping)) return 2;
        require(typeid(mapping) == typeid(MapObject::AttributeMapping));
        return 0;
    }

    QByteArray sectionKey(const ObfSectionInfo& section)
    {
        const auto info = section.container.lock();
        require(bool(info));
        Writer w;
        w.s << quint8(dynamic_cast<const ObfMapSectionInfo*>(&section) ? 1 : 2)
            << qint32(info->version) << quint64(info->creationTimestamp)
            << quint32(section.offset) << quint32(section.length);
        w.str(section.name);
        return w.bytes;
    }

    template<class T> struct Table
    {
        QVector<std::shared_ptr<const T>> values;
        QHash<const T*, quint32> indices;
        quint32 add(const std::shared_ptr<const T>& p)
        {
            require(bool(p));
            const auto it = indices.constFind(p.get());
            if (it != indices.cend()) return *it;
            require(values.size() < int(MaxItems));
            const auto index = quint32(values.size());
            indices.insert(p.get(), index); values.push_back(p);
            return index;
        }
        quint32 index(const std::shared_ptr<const T>& p) const
        {
            const auto it = indices.constFind(p.get()); require(it != indices.cend()); return *it;
        }
    };

    struct Graph
    {
        Table<MapObject> objects;
        Table<ObfSectionInfo> sections;
        Table<MapObject::AttributeMapping> mappings;
        Table<MP::PrimitivesGroup> groups;
        Table<MP::Primitive> primitives;
        QHash<const MapObject::AttributeMapping*, QSet<quint32>> mappingIds;

        void addObject(const std::shared_ptr<const MapObject>& o)
        {
            if (objects.indices.contains(o.get()))
                return;
            objects.add(o);
            const auto kind = objectKind(*o);
            if (kind == 1 || kind == 2)
                sections.add(std::static_pointer_cast<const ObfMapObject>(o)->obfSection);
            mappings.add(o->attributeMapping);
            auto& ids = mappingIds[o->attributeMapping.get()];
            for (auto id : o->attributeIds) ids.insert(id);
            for (auto id : o->additionalAttributeIds) ids.insert(id);
            for (auto id : o->captionsOrder) ids.insert(id);
            for (auto it = o->captions.cbegin(); it != o->captions.cend(); ++it) ids.insert(it.key());
            if (kind == 2)
                for (const auto& types : std::static_pointer_cast<const Road>(o)->pointsTypes)
                    for (auto id : types) ids.insert(id);
            const auto& m = *o->attributeMapping;
            const quint32 required[] = {m.nativeNameAttributeId, m.enNameAttributeId, m.refAttributeId,
                m.naturalCoastlineAttributeId, m.naturalLandAttributeId, m.naturalCoastlineBrokenAttributeId,
                m.naturalCoastlineLineAttributeId, m.onewayAttributeId, m.onewayReverseAttributeId,
                m.layerLowestAttributeId};
            for (auto id : required) if (m.decodeMap.getRef(id)) ids.insert(id);
            for (auto id : m.nameAttributeIds) ids.insert(id);
            if (const auto routing = dynamic_cast<const ObfRoutingSectionAttributeMapping*>(&m))
            {
                for (auto id : routing->refAttributeIds) ids.insert(id);
            }
        }
    };

    const MP::PrimitivesCollection& list(const MP::PrimitivesGroup& g, int k)
    { return k == 0 ? g.polygons : k == 1 ? g.polylines : g.points; }
    MP::PrimitivesCollection& list(MP::PrimitivesGroup& g, int k)
    { return k == 0 ? g.polygons : k == 1 ? g.polylines : g.points; }
    const MP::PrimitivesCollection& list(const MP::PrimitivisedObjects& p, int k)
    { return k == 0 ? p.polygons : k == 1 ? p.polylines : p.points; }
    MP::PrimitivesCollection& list(MP::PrimitivisedObjects& p, int k)
    { return k == 0 ? p.polygons : k == 1 ? p.polylines : p.points; }

    bool belongsToDrawList(MP::PrimitiveType type, int listIndex)
    {
        // Draw order can place either polygons or polylines in either of the first two lists.
        return (type == MP::PrimitiveType::Point) == (listIndex == 2);
    }

    template<class C, class T> void writeSelectedRefs(Writer& w, const C& values, const Table<T>& table)
    {
        QVector<quint32> refs;
        for (const auto& v : values)
        {
            const auto it = table.indices.constFind(v.get());
            if (it != table.indices.cend())
                refs.push_back(*it);
        }
        w.ids(refs);
    }

    void writeObject(Writer& w, const MapObject& o, const Graph& g)
    {
        const auto kind = objectKind(o);
        w.s << quint8(kind) << g.mappings.index(o.attributeMapping);
        if (kind == 1 || kind == 2)
        {
            const auto& obf = static_cast<const ObfMapObject&>(o);
            w.s << g.sections.index(obf.obfSection) << quint64(obf.id.id);
            if (kind == 1)
            {
                const auto& b = static_cast<const BinaryMapObject&>(o);
                require(bool(b.level));
                w.s << quint32(b.level->offset) << qint32(b.level->minZoom) << qint32(b.level->maxZoom)
                    << quint64(b.blockId.id);
            }
            else
            {
                const auto& road = static_cast<const Road&>(o);
                w.s << quint64(road.blockId.id);
                w.count(road.pointsTypes.size());
                for (auto it = road.pointsTypes.cbegin(); it != road.pointsTypes.cend(); ++it)
                { w.s << quint32(it.key()); w.ids(it.value()); }
                w.count(road.restrictions.size());
                for (auto it = road.restrictions.cbegin(); it != road.restrictions.cend(); ++it)
                { w.s << quint64(it.key().id) << qint32(it.value()); }
            }
        }
        w.s << o.isArea << o.isCoastline;
        w.points(o.points31);
        w.count(o.innerPolygonsPoints31.size());
        for (const auto& ring : o.innerPolygonsPoints31) w.points(ring);
        w.area(o.bbox31);
        w.s << qint32(o.labelX) << qint32(o.labelY);
        w.ids(o.attributeIds); w.ids(o.additionalAttributeIds);
        w.count(o.captions.size());
        for (auto it = o.captions.cbegin(); it != o.captions.cend(); ++it)
        { w.s << quint32(it.key()); w.str(it.value()); }
        w.ids(o.captionsOrder);
    }

    void writeSymbol(Writer& w, const MP::Symbol& symbol, const Graph& g)
    {
        const auto text = dynamic_cast<const MP::TextSymbol*>(&symbol);
        const auto icon = dynamic_cast<const MP::IconSymbol*>(&symbol);
        require((text && typeid(symbol) == typeid(MP::TextSymbol)) ||
                (icon && typeid(symbol) == typeid(MP::IconSymbol)));
        w.s << quint8(text ? 1 : 2) << g.primitives.index(symbol.primitive);
        w.point(symbol.location31);
        w.s << qint32(symbol.order) << symbol.drawAlongPath;
        w.strings(symbol.intersectsWith);
        w.f32(symbol.intersectionSizeFactor); w.f32(symbol.intersectionSize); w.f32(symbol.intersectionMargin);
        w.f32(symbol.minDistance); w.f32(symbol.scaleFactor); w.s << symbol.ignoreClick;
        if (text)
        {
            w.str(text->baseValue); w.str(text->value);
            w.s << qint32(text->languageId) << text->drawOnPath;
            w.str(text->topSide); w.s << qint32(text->placement); w.ids(text->additionalPlacements);
            w.s << qint32(text->verticalOffset) << quint32(text->color.argb) << qint32(text->size)
                << qint32(text->shadowRadius) << quint32(text->shadowColor.argb) << qint32(text->wrapWidth)
                << text->isBold << text->isItalic;
            w.str(text->shieldResourceName); w.strings(text->underlayIconResourceNames);
        }
        else
        {
            w.str(icon->resourceName); w.strings(icon->underlayResourceNames); w.strings(icon->overlayResourceNames);
            w.f32(icon->offsetFactor.x); w.f32(icon->offsetFactor.y); w.str(icon->shieldResourceName);
        }
    }

    ObfObjectId restoreId(quint64 stored, const std::shared_ptr<const ObfSectionInfo>& section)
    {
        ObfObjectId id; id.id = stored;
        if (id.isOsmId() || stored == ObfObjectId::invalidId().id) return id;
        // Negative IDs contain a process-local section id in their upper word.
        return ObfObjectId::generateUniqueId(quint32(quint64(0) - stored), section);
    }

    struct CachedObjectsData : IMapObjectsProvider::Data
    {
        CachedObjectsData(TileId tile, ZoomLevel zoom, MapSurfaceType surface,
            const QList<std::shared_ptr<const MapObject>>& objects,
            const QList<std::shared_ptr<const ObfInfo>>& infos)
            : Data(tile, zoom, surface, objects), infos(infos) {}
        // ObfSectionInfo::container is weak. Keep current metadata alive with
        // restored objects even when a collection is refreshed.
        const QList<std::shared_ptr<const ObfInfo>> infos;
    };
}

QByteArray OsmAnd::MapPrimitivesCache::serialize(const MapPrimitivesProvider::Data& data)
{
    try
    {
        require(bool(data.primitivisedObjects) && bool(data.mapObjectsData));
        const auto& p = *data.primitivisedObjects;
        require(bool(p.mapPresentationEnvironment) && bool(p.mapPresentationEnvironment->mapStyle));
        const auto& style = *p.mapPresentationEnvironment->mapStyle;
        Graph g;
        // The cached image supplies raster data. Keep primitives reached from actual symbols,
        // and just the source objects needed for polygon hit testing.
        const auto polygonQueryObjects = p.getPolygonObjects();
        for (const auto& object : polygonQueryObjects)
            g.addObject(object);
        int nonemptyGroupCount = 0;
        for (auto it = p.symbolsGroups.cbegin(); it != p.symbolsGroups.cend(); ++it)
        {
            require(it.key() && it.value() && it.value()->sourceObject);
            if (it.value()->symbols.isEmpty())
                continue;
            ++nonemptyGroupCount;
            g.addObject(it.key());
            g.addObject(it.value()->sourceObject);
            for (const auto& symbol : it.value()->symbols)
            {
                require(symbol && symbol->primitive);
                const auto& primitive = symbol->primitive;
                if (g.primitives.indices.contains(primitive.get()))
                    continue;
                const auto group = primitive->group.lock();
                require(group && primitive->sourceObject == group->sourceObject);
                g.primitives.add(primitive);
                g.groups.add(group);
                g.addObject(group->sourceObject);
            }
        }
        Writer w;
        w.s << qint32(data.tileId.x) << qint32(data.tileId.y) << qint32(data.zoom) << qint32(data.detailedZoom)
            << qint32(data.mapObjectsData->tileSurfaceType);
        w.f64(p.scaleDivisor31ToPixel.x); w.f64(p.scaleDivisor31ToPixel.y); w.s << p.coastlineMistake;

        w.count(g.sections.values.size());
        for (const auto& section : g.sections.values)
        {
            const auto key = sectionKey(*section); w.s << quint32(key.size());
            require(w.s.writeRawData(key.constData(), key.size()) == key.size());
        }
        w.count(g.mappings.values.size());
        for (const auto& mapping : g.mappings.values)
        {
            w.s << quint8(mappingKind(*mapping));
            auto ids = g.mappingIds.value(mapping.get()).values();
            std::sort(ids.begin(), ids.end());
            w.count(ids.size());
            for (auto id : ids)
            {
                require(id < MaxItems);
                const auto entry = mapping->decodeMap.getRef(id); require(entry != nullptr);
                w.s << id; w.str(entry->tag); w.str(entry->value);
            }
        }
        w.count(g.objects.values.size());
        for (const auto& object : g.objects.values) writeObject(w, *object, g);
        writeSelectedRefs(w, data.mapObjectsData->mapObjects, g.objects);

        QHash<int, quint32> definitionIndices;
        QVector<int> definitions;
        for (const auto& primitive : g.primitives.values)
            for (const auto& entry : primitive->evaluationResult.entries)
                if (!definitionIndices.contains(entry.first))
                { definitionIndices.insert(entry.first, definitions.size()); definitions.push_back(entry.first); }
        w.count(definitions.size());
        for (auto id : definitions)
        {
            require(id >= 0 && id < style.getValueDefinitionsCount());
            const auto definition = style.getValueDefinitionById(id); require(bool(definition));
            w.str(definition->name); w.s << qint32(definition->dataType);
        }
        w.count(g.groups.values.size());
        for (const auto& group : g.groups.values) w.s << g.objects.index(group->sourceObject);
        w.count(g.primitives.values.size());
        for (const auto& primitive : g.primitives.values)
        {
            w.s << g.groups.index(primitive->group.lock()) << quint32(primitive->type)
                << quint32(primitive->attributeIdIndex) << qint32(primitive->zOrder) << qint64(primitive->doubledArea);
            w.f32(primitive->detailScaleFactor);
            w.count(primitive->evaluationResult.entries.size());
            for (const auto& entry : primitive->evaluationResult.entries)
            { w.s << definitionIndices.value(entry.first); w.variant(entry.second); }
        }
        for (const auto& group : g.groups.values)
            for (int k = 0; k < 3; ++k) writeSelectedRefs(w, list(*group, k), g.primitives);
        for (int k = 0; k < 3; ++k) writeSelectedRefs(w, list(p, k), g.primitives);
        w.count(nonemptyGroupCount);
        for (auto it = p.symbolsGroups.cbegin(); it != p.symbolsGroups.cend(); ++it)
        {
            const auto& group = *it.value();
            if (group.symbols.isEmpty())
                continue;
            w.s << g.objects.index(it.key()) << g.objects.index(group.sourceObject) << group.canBeShownWithoutIcon;
            w.count(group.symbols.size());
            for (const auto& symbol : group.symbols) { require(bool(symbol)); writeSymbol(w, *symbol, g); }
        }
        w.s << (p.hasEmptySymbolGroups || nonemptyGroupCount < p.symbolsGroups.size());
        w.count(polygonQueryObjects.size());
        for (const auto& object : polygonQueryObjects)
            w.s << g.objects.index(object);
        require(w.s.status() == QDataStream::Ok && quint32(w.bytes.size()) <= MaxBytes);
        Writer result;
        result.s << Version << quint32(w.bytes.size());
        const auto digest = QCryptographicHash::hash(w.bytes, QCryptographicHash::Sha256);
        result.s.writeRawData(digest.constData(), digest.size());
        require(result.s.status() == QDataStream::Ok && result.bytes.size() == HeaderSize);
        uLongf compressedSize = compressBound(w.bytes.size());
        result.bytes.resize(HeaderSize + int(compressedSize));
        require(compress2(reinterpret_cast<Bytef*>(result.bytes.data() + HeaderSize), &compressedSize,
            reinterpret_cast<const Bytef*>(w.bytes.constData()), w.bytes.size(), Z_BEST_SPEED) == Z_OK);
        result.bytes.resize(HeaderSize + int(compressedSize));
        return result.bytes;
    }
    catch (const Invalid&) { return {}; }
    catch (const std::bad_alloc&) { return {}; }
}

bool OsmAnd::MapPrimitivesCache::deserialize(
    const QByteArray& bytes, const MapPrimitivesProvider& provider,
    const IMapTiledDataProvider::Request& request,
    std::shared_ptr<IMapObjectsProvider::Data>& objects,
    std::shared_ptr<MapPrimitiviser::PrimitivisedObjects>& primitives)
{
    objects.reset(); primitives.reset();
    try
    {
        require(bytes.size() > HeaderSize);
        Reader header(bytes);
        require(header.u32() == Version);
        const auto length = header.u32();
        require(length > 0 && length <= MaxBytes);
        const auto storedSize = bytes.size() - HeaderSize;
        require(uLong(storedSize) <= compressBound(length));
        QByteArray payload(int(length), Qt::Uninitialized);
        uLongf decodedSize = length;
        uLong consumed = storedSize;
        require(uncompress2(reinterpret_cast<Bytef*>(payload.data()), &decodedSize,
            reinterpret_cast<const Bytef*>(bytes.constData() + HeaderSize), &consumed) == Z_OK);
        require(decodedSize == length && consumed == uLong(storedSize));
        const auto digest = QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
        require(header.buffer.read(digest.size()) == digest);
        Reader r(payload);
        const auto x = r.i32(); const auto y = r.i32(); const auto zoom = r.i32(); const auto detailed = r.i32();
        require(x == request.tileId.x && y == request.tileId.y && zoom == request.zoom &&
            detailed == (request.detailedZoom == InvalidZoomLevel ? request.zoom : request.detailedZoom));
        const auto surface = r.i32(); require(surface >= -1 && surface <= 2);
        const auto scaleX = r.f64(); const auto scaleY = r.f64();
        require(std::isfinite(scaleX) && std::isfinite(scaleY) &&
            ((scaleX > 0 && scaleY > 0) || (scaleX == -1 && scaleY == -1)));
        const auto coastlineMistake = r.boolean();
        require(bool(provider.primitiviser) && bool(provider.primitiviser->environment));
        const auto& environment = provider.primitiviser->environment;
        require(bool(environment->mapStyle));
        const auto& style = *environment->mapStyle;

        QVector<QByteArray> sectionKeys;
        for (auto n = r.count(4); n > 0; --n)
        {
            const auto size = r.count(1); require(size > 0);
            QByteArray key(int(size), '\0');
            require(r.s.readRawData(key.data(), size) == int(size)); sectionKeys.push_back(key);
        }
        QVector<std::shared_ptr<const ObfSectionInfo>> sections;
        QList<std::shared_ptr<const ObfInfo>> infos;
        if (!sectionKeys.isEmpty())
        {
            const auto obf = std::dynamic_pointer_cast<const ObfMapObjectsProvider>(provider.mapObjectsProvider);
            require(bool(obf) && bool(obf->obfsCollection));
            QHash<QByteArray, std::shared_ptr<const ObfSectionInfo>> current;
            auto add = [&](const std::shared_ptr<const ObfSectionInfo>& section)
            {
                const auto key = sectionKey(*section);
                // Ambiguous source identity is a miss rather than silently
                // assigning generated object IDs to a different source file.
                if (current.contains(key) && current.value(key) != section) current[key].reset();
                else current.insert(key, section);
            };
            for (const auto& file : obf->obfsCollection->getObfFiles())
            {
                if (request.queryController && request.queryController->isAborted()) throw Invalid();
                const auto info = file->obfInfo; if (!info) continue;
                infos.push_back(info);
                for (const auto& section : info->mapSections) add(section.shared_ptr());
                for (const auto& section : info->routingSections) add(section.shared_ptr());
            }
            for (const auto& key : sectionKeys)
            { const auto section = current.value(key); require(bool(section)); sections.push_back(section); }
        }
        QVector<std::shared_ptr<const MapObject::AttributeMapping>> mappings;
        for (auto n = r.count(5); n > 0; --n)
        {
            const auto kind = r.u8(); require(kind <= 2);
            // OBF mappings begin with sparse slots, and a large attribute ID
            // expands them independently of the number of serialized entries.
            r.account(kind == 0 ? 0 : 4096 * 32);
            std::shared_ptr<MapObject::AttributeMapping> mapping;
            if (kind == 1) mapping.reset(new ObfMapSectionAttributeMapping());
            else if (kind == 2) mapping.reset(new ObfRoutingSectionAttributeMapping());
            else mapping.reset(new MapObject::AttributeMapping());
            for (auto count = r.count(12); count > 0; --count)
            {
                const auto id = r.u32(); require(id < MaxItems && !mapping->decodeMap.getRef(id));
                if (id >= mapping->decodeMap.size()) r.account((quint64(id) + 1 - mapping->decodeMap.size()) * 32);
                const auto tag = r.str(); const auto value = r.str(); mapping->registerMapping(id, tag, value);
            }
            mapping->verifyRequiredMappingRegistered(); mappings.push_back(mapping);
        }
        QVector<std::shared_ptr<const MapObject>> objectTable;
        for (auto n = r.count(5); n > 0; --n)
        {
            if (request.queryController && request.queryController->isAborted()) throw Invalid();
            const auto kind = r.u8(); require(kind <= 4);
            const auto mapping = r.ref(mappings);
            std::shared_ptr<MapObject> object;
            if (kind == 1 || kind == 2)
            {
                const auto section = r.ref(sections); const auto id = restoreId(r.u64(), section);
                if (kind == 1)
                {
                    const auto mapSection = std::dynamic_pointer_cast<const ObfMapSectionInfo>(section);
                    require(bool(mapSection) && dynamic_cast<const ObfMapSectionAttributeMapping*>(mapping.get()));
                    const auto offset = r.u32(); const auto min = r.i32(); const auto max = r.i32();
                    std::shared_ptr<const ObfMapSectionLevel> level;
                    for (const auto& candidate : mapSection->levels)
                        if (candidate->offset == offset && candidate->minZoom == min && candidate->maxZoom == max)
                        { level = candidate.shared_ptr(); break; }
                    require(bool(level));
                    auto binary = std::shared_ptr<BinaryMapObject>(new BinaryMapObject(mapSection, level));
                    binary->id = id; binary->blockId.id = r.u64();
                    if (binary->blockId.id != ObfMapSectionDataBlockId::invalidId().id)
                        binary->blockId.sectionRuntimeGeneratedId = section->runtimeGeneratedId;
                    object = binary;
                }
                else
                {
                    const auto routingSection = std::dynamic_pointer_cast<const ObfRoutingSectionInfo>(section);
                    require(bool(routingSection)
                        && dynamic_cast<const ObfRoutingSectionAttributeMapping*>(mapping.get()));
                    auto road = std::shared_ptr<Road>(new Road(routingSection));
                    road->id = id; road->blockId.id = r.u64();
                    if (road->blockId.id != ObfRoutingSectionDataBlockId::invalidId().id)
                        road->blockId.sectionRuntimeGeneratedId = section->runtimeGeneratedId;
                    for (auto count = r.count(8); count > 0; --count)
                    {
                        const auto pointIndex = r.u32(); require(!road->pointsTypes.contains(pointIndex));
                        road->pointsTypes.insert(pointIndex, r.ids<QVector<quint32>>());
                    }
                    for (auto count = r.count(12); count > 0; --count)
                    {
                        const auto to = restoreId(r.u64(), section); const auto restriction = r.i32();
                        require(restriction >= -1 && restriction <= 7 && !road->restrictions.contains(to));
                        road->restrictions.insert(to, static_cast<RoadRestriction>(restriction));
                    }
                    object = road;
                }
            }
            else if (kind == 3) object.reset(new MP::CoastlineMapObject());
            else if (kind == 4) object.reset(new MP::SurfaceMapObject());
            else object.reset(new MapObject());
            object->attributeMapping = mapping;
            object->isArea = r.boolean(); object->isCoastline = r.boolean(); object->points31 = r.points();
            for (auto count = r.count(4); count > 0; --count) object->innerPolygonsPoints31.push_back(r.points());
            object->bbox31 = r.area(); object->labelX = r.i32(); object->labelY = r.i32();
            object->attributeIds = r.ids<QVector<quint32>>();
            object->additionalAttributeIds = r.ids<QVector<quint32>>();
            for (auto count = r.count(8); count > 0; --count)
            {
                const auto id = r.u32(); require(!object->captions.contains(id));
                object->captions.insert(id, r.str());
            }
            object->captionsOrder = r.ids<QList<quint32>>();
            const auto checkIds = [&](const auto& ids) { for (auto id : ids) require(mapping->decodeMap.getRef(id)); };
            checkIds(object->attributeIds); checkIds(object->additionalAttributeIds); checkIds(object->captionsOrder);
            for (auto it = object->captions.cbegin(); it != object->captions.cend(); ++it)
                require(mapping->decodeMap.getRef(it.key()));
            if (kind == 2)
            {
                const auto road = std::static_pointer_cast<Road>(object);
                for (auto it = road->pointsTypes.cbegin(); it != road->pointsTypes.cend(); ++it)
                { require(it.key() < quint32(road->points31.size())); checkIds(it.value()); }
            }
            objectTable.push_back(object);
        }
        QList<std::shared_ptr<const MapObject>> sourceObjects;
        for (auto n = r.count(4); n > 0; --n) sourceObjects.push_back(r.ref(objectTable));
        QVector<int> definitionIds;
        QSet<int> seenDefinitions;
        for (auto n = r.count(8); n > 0; --n)
        {
            const auto name = r.str();
            const auto type = r.i32();
            const auto id = style.getValueDefinitionIdByName(name);
            require(id >= 0 && id < style.getValueDefinitionsCount() && !seenDefinitions.contains(id));
            const auto definition = style.getValueDefinitionById(id);
            require(bool(definition) && int(definition->dataType) == type);
            definitionIds.push_back(id); seenDefinitions.insert(id);
        }
        auto result = std::shared_ptr<MP::PrimitivisedObjects>(new MP::PrimitivisedObjects(
            environment, nullptr, request.zoom, PointD(scaleX, scaleY)));
        result->coastlineMistake = coastlineMistake;
        QVector<std::shared_ptr<MP::PrimitivesGroup>> groups;
        for (auto n = r.count(4); n > 0; --n)
        {
            auto group = std::shared_ptr<MP::PrimitivesGroup>(new MP::PrimitivesGroup(r.ref(objectTable)));
            groups.push_back(group); result->primitivesGroups.push_back(group);
        }
        QVector<std::shared_ptr<const MP::Primitive>> primitiveTable;
        for (auto n = r.count(32); n > 0; --n)
        {
            const auto group = r.ref(groups); const auto type = r.u32(); const auto attribute = r.u32();
            require(type >= 1 && type <= 3 && attribute < quint32(group->sourceObject->attributeIds.size()));
            const auto zOrder = r.i32(); const auto area = r.i64(); const auto scale = r.f32();
            require(std::isfinite(scale) && scale > 0);
            MapStyleEvaluationResult evaluation;
            QSet<int> seenValues;
            for (auto count = r.count(5); count > 0; --count)
            {
                const auto id = r.ref(definitionIds); require(!seenValues.contains(id));
                evaluation.setValue(id, r.variant()); seenValues.insert(id);
            }
            auto primitive = std::shared_ptr<MP::Primitive>(new MP::Primitive(
                group, static_cast<MP::PrimitiveType>(type), attribute, evaluation, scale));
            primitive->zOrder = zOrder; primitive->doubledArea = area; primitiveTable.push_back(primitive);
        }
        for (const auto& group : groups)
            for (int k = 0; k < 3; ++k)
                for (auto n = r.count(4); n > 0; --n)
                {
                    const auto p = r.ref(primitiveTable); require(p->group.lock() == group);
                    require(belongsToDrawList(p->type, k));
                    list(*group, k).push_back(p);
                }
        for (int k = 0; k < 3; ++k)
            for (auto n = r.count(4); n > 0; --n)
            {
                const auto p = r.ref(primitiveTable); require(belongsToDrawList(p->type, k));
                list(*result, k).push_back(p);
            }
        QBitArray seenSymbolGroups(objectTable.size());
        for (auto n = r.count(13); n > 0; --n)
        {
            const auto keyIndex = r.u32();
            require(keyIndex < quint32(objectTable.size()) && !seenSymbolGroups.testBit(int(keyIndex)));
            seenSymbolGroups.setBit(int(keyIndex));
            const auto source = r.ref(objectTable);
            const auto withoutIcon = r.boolean();
            const auto symbolCount = r.count(5);
            require(symbolCount > 0);
            auto group = std::shared_ptr<MP::SymbolsGroup>(new MP::SymbolsGroup(source, withoutIcon));
            for (auto count = symbolCount; count > 0; --count)
            {
                const auto kind = r.u8();
                require(kind == 1 || kind == 2);
                const auto primitive = r.ref(primitiveTable);
                std::shared_ptr<MP::Symbol> symbol;
                if (kind == 1) symbol.reset(new MP::TextSymbol(primitive));
                else symbol.reset(new MP::IconSymbol(primitive));
                symbol->location31 = r.point(); symbol->order = r.i32(); symbol->drawAlongPath = r.boolean();
                for (const auto& text : r.strings()) symbol->intersectsWith.insert(text);
                symbol->intersectionSizeFactor = r.f32();
                symbol->intersectionSize = r.f32();
                symbol->intersectionMargin = r.f32();
                symbol->minDistance = r.f32(); symbol->scaleFactor = r.f32(); symbol->ignoreClick = r.boolean();
                if (kind == 1)
                {
                    const auto text = std::static_pointer_cast<MP::TextSymbol>(symbol);
                    text->baseValue = r.str(); text->value = r.str(); const auto language = r.i32();
                    require(language >= -1 && language <= 1); text->languageId = static_cast<LanguageId>(language);
                    text->drawOnPath = r.boolean(); text->topSide = r.str(); const auto placement = r.i32();
                    require(placement >= 0 && placement <= 2);
                    text->placement = static_cast<MP::TextSymbol::Placement>(placement);
                    for (auto c = r.count(4); c > 0; --c)
                    {
                        const auto v = r.u32();
                        require(v <= 2);
                        text->additionalPlacements.push_back(static_cast<MP::TextSymbol::Placement>(v));
                    }
                    text->verticalOffset = r.i32(); text->color.argb = r.u32(); text->size = r.i32();
                    text->shadowRadius = r.i32(); text->shadowColor.argb = r.u32(); text->wrapWidth = r.i32();
                    text->isBold = r.boolean(); text->isItalic = r.boolean(); text->shieldResourceName = r.str();
                    text->underlayIconResourceNames = r.strings();
                }
                else
                {
                    const auto icon = std::static_pointer_cast<MP::IconSymbol>(symbol);
                    icon->resourceName = r.str();
                    icon->underlayResourceNames = r.strings();
                    icon->overlayResourceNames = r.strings();
                    const auto offsetX = r.f32();
                    const auto offsetY = r.f32();
                    icon->offsetFactor = PointF(offsetX, offsetY);
                    icon->shieldResourceName = r.str();
                }
                group->symbols.push_back(symbol);
            }
            result->symbolsGroups.insert(objectTable[int(keyIndex)], group);
        }
        result->hasEmptySymbolGroups = r.boolean();
        result->isSymbolsOnly = true;
        QBitArray seenPolygons(objectTable.size());
        for (auto n = r.count(4); n > 0; --n)
        {
            const auto index = r.u32();
            require(index < quint32(objectTable.size()) && !seenPolygons.testBit(int(index)));
            const auto& object = objectTable[int(index)];
            require(dynamic_cast<const ObfMapObject*>(object.get()));
            seenPolygons.setBit(int(index));
            result->polygonQueryObjects.push_back(object);
        }
        require(r.atEnd() && (!request.queryController || !request.queryController->isAborted()));
        objects.reset(new CachedObjectsData(
            request.tileId, request.zoom, static_cast<MapSurfaceType>(surface), sourceObjects, infos));
        primitives = result;
        return true;
    }
    catch (const Invalid&) { return false; }
    catch (const std::bad_alloc&) { return false; }
}
