#include "MapPrimitiviser_P.h"
#include "MapPrimitiviser.h"

#include "ignore_warnings_on_external_includes.h"
#include <QDataStream>
#include <QSet>
#include "restore_internal_warnings.h"

#include <algorithm>
#include <cstring>
#include <limits>

#include "MapPresentationEnvironment.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleValueDefinition.h"
#include "IMapStyle.h"
#include "ObfInfo.h"
#include "ObfMapSectionInfo.h"
#include "MapObject.h"
#include "BinaryMapObject.h"

// Map symbols of one tile for the map tile disk cache.
//
// Only what can reach the screen is kept:
//  - text not on a path: the best one (lowest order) per cell of a 16x16 grid over the tile, since two captions
//    closer than 1/16 of a tile (less than a line of text) overlap and the renderer keeps only the first one;
//    text of a group with an icon (a POI or station name) stays with its icon;
//  - text on a path: all of it, the renderer joins street segments before it chooses;
//  - icons: all of them;
//  - then a symbol group is dropped when a group of lower order and a shared intersection class is closer than
//    a quarter of their summed sizes: such pairs overlap at any rotation and any reasonable font scale.
// Objects keep their ids, tags and bbox (a tap finds the object, its name comes from the drawn caption);
// geometry is kept only for objects with text on a path.

namespace
{
    const quint32 kMagic = 0x4F534D53; // 'OSMS'
    const int kGridShift = 4;          // 16x16 cells per tile

    inline void writeDouble(QDataStream& s, const double v)
    {
        quint64 bits;
        memcpy(&bits, &v, sizeof(bits));
        s << bits;
    }

    inline double readDouble(QDataStream& s)
    {
        quint64 bits;
        s >> bits;
        double v;
        memcpy(&v, &bits, sizeof(v));
        return v;
    }

    void writePoints(QDataStream& s, const QVector<OsmAnd::PointI>& points)
    {
        s << static_cast<quint32>(points.size());
        for (const auto& p : points)
            s << static_cast<qint32>(p.x) << static_cast<qint32>(p.y);
    }

    bool readPoints(QDataStream& s, QVector<OsmAnd::PointI>& points)
    {
        quint32 count;
        s >> count;
        if (s.status() != QDataStream::Ok || count > (1u << 24))
            return false;
        points.resize(count);
        for (auto& p : points)
        {
            qint32 x, y;
            s >> x >> y;
            p.x = x;
            p.y = y;
        }
        return s.status() == QDataStream::Ok;
    }

    // equal order: text, then position, so the choice does not depend on hash order
    bool isBetterCaption(const OsmAnd::MapPrimitiviser::TextSymbol& l, const OsmAnd::MapPrimitiviser::TextSymbol& r)
    {
        if (l.order != r.order)
            return l.order < r.order;
        if (l.value != r.value)
            return l.value < r.value;
        if (l.location31.x != r.location31.x)
            return l.location31.x < r.location31.x;
        return l.location31.y < r.location31.y;
    }
}

QString OsmAnd::MapPrimitiviser_P::getDiskCacheSectionKey(const ObfMapSectionInfo& section)
{
    const auto info = section.container.lock();
    return QStringLiteral("%1@%2+%3/%4")
        .arg(section.name)
        .arg(section.offset)
        .arg(section.length)
        .arg(info ? info->creationTimestamp : 0);
}

QByteArray OsmAnd::MapPrimitiviser_P::serializeSymbols(
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& prim,
    const MapSurfaceType surfaceType)
{
    typedef MapPrimitiviser::Symbol Symbol;
    typedef MapPrimitiviser::TextSymbol TextSymbol;

    // 1. symbols that can reach the screen
    QSet<const Symbol*> keptSymbols;
    {
        const int shift = qMax(0, 31 - static_cast<int>(prim->zoom) - kGridShift);
        QHash<quint64, const TextSymbol*> bestInCell;
        for (const auto& group : prim->symbolsGroups)
        {
            bool hasIcon = false;
            for (const auto& symbol : group->symbols)
                hasIcon = hasIcon || !std::dynamic_pointer_cast<const TextSymbol>(symbol);
            for (const auto& symbol : group->symbols)
            {
                const auto text = std::dynamic_pointer_cast<const TextSymbol>(symbol);
                if (!text || text->drawOnPath || hasIcon)
                {
                    keptSymbols.insert(symbol.get());
                    continue;
                }
                const quint64 cell = (static_cast<quint64>(static_cast<quint32>(text->location31.x >> shift)) << 32)
                    | static_cast<quint32>(text->location31.y >> shift);
                auto& best = bestInCell[cell];
                if (!best || isBetterCaption(*text, *best))
                    best = text.get();
            }
        }
        for (const auto text : bestInCell)
            keptSymbols.insert(text);
    }

    // 2. groups that overlap a group of lower order at any rotation
    QSet<const MapObject*> keptGroups, keptObjects, pathObjects;
    {
        struct Candidate
        {
            const MapObject* key;
            const MapObject* source;
            double x, y, size;
            int order;
            QSet<QString> classes;
            bool onPath;
        };
        QVector<Candidate> candidates;
        const auto density = prim->mapPresentationEnvironment->displayDensityFactor;
        for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
        {
            const auto& group = it.value();
            Candidate c;
            c.key = it.key().get();
            c.source = group->sourceObject.get();
            c.onPath = false;
            c.size = 0;
            c.order = std::numeric_limits<int>::max();
            const Symbol* first = nullptr;
            for (const auto& symbol : group->symbols)
            {
                if (!keptSymbols.contains(symbol.get()))
                    continue;
                if (!first)
                    first = symbol.get();
                const auto text = std::dynamic_pointer_cast<const TextSymbol>(symbol);
                if (symbol->drawAlongPath || (text && text->drawOnPath))
                    c.onPath = true;
                c.size = qMax(c.size, text ? static_cast<double>(text->size) : 20.0 * density);
                c.order = qMin(c.order, symbol->order);
                c.classes.unite(symbol->intersectsWith);
            }
            if (!first)
                continue;
            c.x = first->location31.x / prim->scaleDivisor31ToPixel.x;
            c.y = first->location31.y / prim->scaleDivisor31ToPixel.y;
            candidates.push_back(c);
        }
        std::stable_sort(candidates.begin(), candidates.end(),
            []
            (const Candidate& l, const Candidate& r) -> bool
            {
                if (l.order != r.order)
                    return l.order < r.order;
                if (l.x != r.x)
                    return l.x < r.x;
                return l.y < r.y;
            });
        QVector<int> kept;
        for (int i = 0; i < candidates.size(); i++)
        {
            const auto& c = candidates[i];
            bool overlaps = false;
            if (!c.onPath && !c.classes.isEmpty())
            {
                for (const int j : kept)
                {
                    const auto& k = candidates[j];
                    if (k.onPath || !k.classes.intersects(c.classes))
                        continue;
                    const double limit = (c.size + k.size) / 4.0;
                    const double dx = c.x - k.x, dy = c.y - k.y;
                    if (dx * dx + dy * dy < limit * limit)
                    {
                        overlaps = true;
                        break;
                    }
                }
            }
            if (overlaps)
                continue;
            kept.push_back(i);
            keptGroups.insert(c.key);
            keptObjects.insert(c.key);
            keptObjects.insert(c.source);
            if (c.onPath)
            {
                pathObjects.insert(c.key);
                pathObjects.insert(c.source);
            }
        }
    }

    // 3. objects and primitive groups the kept symbols refer to
    MapPrimitiviser::PrimitivesGroupsCollection groups;
    for (const auto& group : prim->primitivesGroups)
        if (keptObjects.contains(group->sourceObject.get()))
            groups.push_back(group);

    QHash<const MapObject*, quint32> objectIndex;
    QList< std::shared_ptr<const MapObject> > objects;
    const auto addObject =
        [&objectIndex, &objects]
        (const std::shared_ptr<const MapObject>& object)
        {
            if (objectIndex.contains(object.get()))
                return;
            objectIndex.insert(object.get(), objects.size());
            objects.push_back(object);
        };
    for (const auto& group : groups)
        addObject(group->sourceObject);
    for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
    {
        if (!keptGroups.contains(it.key().get()))
            continue;
        addObject(it.key());
        addObject(it.value()->sourceObject);
    }

    QHash<const MapPrimitiviser::Primitive*, quint64> primitiveRef;
    for (int gi = 0; gi < groups.size(); gi++)
    {
        const MapPrimitiviser::PrimitivesCollection* lists[3] = { &groups[gi]->polygons, &groups[gi]->polylines, &groups[gi]->points };
        for (quint64 k = 0; k < 3; k++)
            for (int i = 0; i < lists[k]->size(); i++)
                primitiveRef.insert((*lists[k])[i].get(), (static_cast<quint64>(gi) << 34) | (k << 32) | static_cast<quint64>(i));
    }

    QByteArray out;
    QDataStream s(&out, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_5_6);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);
    s << kMagic << static_cast<qint32>(prim->zoom) << static_cast<qint32>(surfaceType) << prim->coastlineMistake;
    writeDouble(s, prim->scaleDivisor31ToPixel.x);
    writeDouble(s, prim->scaleDivisor31ToPixel.y);

    QStringList sectionKeys;
    QHash<const ObfMapSectionInfo*, qint32> sectionIndex;
    for (const auto& object : objects)
    {
        if (const auto binary = std::dynamic_pointer_cast<const BinaryMapObject>(object))
        {
            if (!sectionIndex.contains(binary->section.get()))
            {
                sectionIndex.insert(binary->section.get(), sectionKeys.size());
                sectionKeys.push_back(getDiskCacheSectionKey(*binary->section));
            }
        }
    }
    s << sectionKeys;
    s << static_cast<quint32>(objects.size());
    for (const auto& object : objects)
    {
        qint32 kind = -1;
        const auto binary = std::dynamic_pointer_cast<const BinaryMapObject>(object);
        if (binary)
            kind = 0;
        else if (std::dynamic_pointer_cast<const MapPrimitiviser::SurfaceMapObject>(object))
            kind = 1;
        else if (std::dynamic_pointer_cast<const MapPrimitiviser::CoastlineMapObject>(object))
            kind = 2;
        if (kind < 0)
            return QByteArray();  // an object that can not be restored: this tile is not cached
        s << kind;
        if (binary)
        {
            s << sectionIndex.value(binary->section.get())
              << static_cast<qint32>(binary->level->minZoom) << static_cast<qint32>(binary->level->maxZoom)
              << static_cast<quint64>(binary->id.id) << static_cast<quint64>(binary->blockId.id);
        }
        s << object->attributeIds << object->additionalAttributeIds << object->isArea << object->isCoastline;
        if (pathObjects.contains(object.get()))
        {
            writePoints(s, object->points31);
            s << static_cast<quint32>(object->innerPolygonsPoints31.size());
            for (const auto& inner : object->innerPolygonsPoints31)
                writePoints(s, inner);
        }
        else
        {
            writePoints(s, QVector<PointI>());
            s << static_cast<quint32>(0);
        }
        s << static_cast<qint32>(object->bbox31.top()) << static_cast<qint32>(object->bbox31.left())
          << static_cast<qint32>(object->bbox31.bottom()) << static_cast<qint32>(object->bbox31.right())
          << static_cast<qint32>(object->labelX) << static_cast<qint32>(object->labelY);
    }

    // value definition ids are given out per process: names go first
    const auto& mapStyle = prim->mapPresentationEnvironment->mapStyle;
    QHash<qint32, qint32> definitionIndex;
    QStringList definitionNames;
    for (const auto& group : groups)
    {
        for (const auto* list : { &group->polygons, &group->polylines, &group->points })
        {
            for (const auto& primitive : *list)
            {
                for (const auto& entry : primitive->evaluationResult.entries)
                {
                    if (definitionIndex.contains(entry.first))
                        continue;
                    const auto& definition = mapStyle->getValueDefinitionRefById(entry.first);
                    if (!definition)
                        return QByteArray();
                    definitionIndex.insert(entry.first, definitionNames.size());
                    definitionNames.push_back(definition->name);
                }
            }
        }
    }
    s << definitionNames;
    s << static_cast<quint32>(groups.size());
    for (const auto& group : groups)
    {
        s << objectIndex.value(group->sourceObject.get());
        for (const auto* list : { &group->polygons, &group->polylines, &group->points })
        {
            s << static_cast<quint32>(list->size());
            for (const auto& primitive : *list)
            {
                s << static_cast<quint32>(primitive->type) << static_cast<quint32>(primitive->attributeIdIndex)
                  << static_cast<qint32>(primitive->zOrder) << static_cast<qint64>(primitive->doubledArea)
                  << primitive->detailScaleFactor;
                s << static_cast<quint32>(primitive->evaluationResult.entries.size());
                for (const auto& entry : primitive->evaluationResult.entries)
                    s << definitionIndex.value(entry.first) << entry.second;
            }
        }
    }

    s << static_cast<quint32>(keptGroups.size());
    for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
    {
        if (!keptGroups.contains(it.key().get()))
            continue;
        const auto& group = it.value();
        s << objectIndex.value(it.key().get()) << objectIndex.value(group->sourceObject.get()) << group->canBeShownWithoutIcon;
        quint32 count = 0;
        for (const auto& symbol : group->symbols)
            if (keptSymbols.contains(symbol.get()))
                count++;
        s << count;
        for (const auto& symbol : group->symbols)
        {
            if (!keptSymbols.contains(symbol.get()))
                continue;
            const auto ref = primitiveRef.constFind(symbol->primitive.get());
            if (ref == primitiveRef.cend())
                return QByteArray();
            const auto text = std::dynamic_pointer_cast<const TextSymbol>(symbol);
            const auto icon = std::dynamic_pointer_cast<const MapPrimitiviser::IconSymbol>(symbol);
            if (!text && !icon)
                return QByteArray();
            s << static_cast<qint32>(text ? 1 : 2) << *ref;
            s << static_cast<qint32>(symbol->location31.x) << static_cast<qint32>(symbol->location31.y)
              << static_cast<qint32>(symbol->order) << symbol->drawAlongPath << symbol->intersectsWith
              << symbol->intersectionSizeFactor << symbol->intersectionSize << symbol->intersectionMargin
              << symbol->minDistance << symbol->scaleFactor << symbol->ignoreClick;
            if (text)
            {
                s << text->baseValue << text->value << static_cast<qint32>(text->languageId) << text->drawOnPath
                  << text->topSide << static_cast<qint32>(text->placement);
                s << static_cast<quint32>(text->additionalPlacements.size());
                for (const auto placement : text->additionalPlacements)
                    s << static_cast<qint32>(placement);
                s << static_cast<qint32>(text->verticalOffset) << static_cast<quint32>(text->color.argb)
                  << static_cast<qint32>(text->size) << static_cast<qint32>(text->shadowRadius)
                  << static_cast<quint32>(text->shadowColor.argb) << static_cast<qint32>(text->wrapWidth)
                  << text->isBold << text->isItalic << text->shieldResourceName << text->underlayIconResourceNames;
            }
            else
            {
                s << icon->resourceName << icon->underlayResourceNames << icon->overlayResourceNames
                  << icon->offsetFactor.x << icon->offsetFactor.y << icon->shieldResourceName;
            }
        }
    }
    if (s.status() != QDataStream::Ok)
        return QByteArray();
    return qCompress(out, 1);
}

bool OsmAnd::MapPrimitiviser_P::deserializeSymbols(
    const QByteArray& compressed,
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const QHash< QString, std::shared_ptr<const ObfMapSectionInfo> >& sections,
    std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& outPrimitivised,
    QList< std::shared_ptr<const MapObject> >& outMapObjects,
    MapSurfaceType& outSurfaceType)
{
    const auto data = qUncompress(compressed);
    if (data.isEmpty())
        return false;
    QDataStream s(data);
    s.setVersion(QDataStream::Qt_5_6);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);
    quint32 magic;
    qint32 zoom, surface;
    bool coastlineMistake;
    s >> magic;
    if (magic != kMagic)
        return false;
    s >> zoom >> surface >> coastlineMistake;
    PointD scaleDivisor;
    scaleDivisor.x = readDouble(s);
    scaleDivisor.y = readDouble(s);

    QStringList sectionKeys;
    s >> sectionKeys;
    QVector< std::shared_ptr<const ObfMapSectionInfo> > objectSections(sectionKeys.size());
    for (int i = 0; i < sectionKeys.size(); i++)
    {
        const auto section = sections.value(sectionKeys[i]);
        // the attribute mapping is read with the first map block: until a normal read happened, it is a miss
        if (!section || !section->getAttributeMapping())
            return false;
        objectSections[i] = section;
    }

    quint32 objectCount;
    s >> objectCount;
    if (s.status() != QDataStream::Ok || objectCount > (1u << 20))
        return false;
    QVector< std::shared_ptr<MapObject> > objects(objectCount);
    for (auto& object : objects)
    {
        qint32 kind;
        s >> kind;
        if (kind == 0)
        {
            qint32 sectionIndex, minZoom, maxZoom;
            quint64 id, blockId;
            s >> sectionIndex >> minZoom >> maxZoom >> id >> blockId;
            if (sectionIndex < 0 || sectionIndex >= objectSections.size())
                return false;
            const auto& section = objectSections[sectionIndex];
            std::shared_ptr<const ObfMapSectionLevel> level;
            for (const auto& l : section->levels)
            {
                if (static_cast<int>(l->minZoom) == minZoom && static_cast<int>(l->maxZoom) == maxZoom)
                {
                    level = l.shared_ptr();
                    break;
                }
            }
            if (!level)
                return false;
            const std::shared_ptr<BinaryMapObject> binary(new BinaryMapObject(section, level));
            binary->id.id = id;
            binary->blockId.id = blockId;
            object = binary;
        }
        else if (kind == 1)
            object.reset(new MapPrimitiviser::SurfaceMapObject());
        else if (kind == 2)
            object.reset(new MapPrimitiviser::CoastlineMapObject());
        else
            return false;
        s >> object->attributeIds >> object->additionalAttributeIds >> object->isArea >> object->isCoastline;
        if (!readPoints(s, object->points31))
            return false;
        quint32 innerCount;
        s >> innerCount;
        if (innerCount > (1u << 16))
            return false;
        for (quint32 i = 0; i < innerCount; i++)
        {
            QVector<PointI> points;
            if (!readPoints(s, points))
                return false;
            object->innerPolygonsPoints31.push_back(qMove(points));
        }
        qint32 top, left, bottom, right, labelX, labelY;
        s >> top >> left >> bottom >> right >> labelX >> labelY;
        object->bbox31 = AreaI(top, left, bottom, right);
        object->labelX = labelX;
        object->labelY = labelY;
    }
    if (s.status() != QDataStream::Ok)
        return false;

    const std::shared_ptr<MapPrimitiviser::PrimitivisedObjects> prim(new MapPrimitiviser::PrimitivisedObjects(
        env, nullptr, static_cast<ZoomLevel>(zoom), scaleDivisor));
    prim->coastlineMistake = coastlineMistake;
    const int definitionsCount = env->mapStyle->getValueDefinitionsCount();

    QStringList definitionNames;
    s >> definitionNames;
    QVector<qint32> definitionIds(definitionNames.size());
    for (int i = 0; i < definitionNames.size(); i++)
    {
        definitionIds[i] = env->mapStyle->getValueDefinitionIdByName(definitionNames[i]);
        if (definitionIds[i] < 0)
            return false;
    }

    QVector< std::shared_ptr<MapPrimitiviser::PrimitivesGroup> > groups;
    quint32 groupCount;
    s >> groupCount;
    if (s.status() != QDataStream::Ok || groupCount > objectCount)
        return false;
    for (quint32 gi = 0; gi < groupCount; gi++)
    {
        quint32 objectIndex;
        s >> objectIndex;
        if (objectIndex >= objectCount)
            return false;
        const std::shared_ptr<MapPrimitiviser::PrimitivesGroup> group(new MapPrimitiviser::PrimitivesGroup(objects[objectIndex]));
        for (int k = 0; k < 3; k++)
        {
            quint32 count;
            s >> count;
            if (s.status() != QDataStream::Ok || count > (1u << 16))
                return false;
            auto& list = k == 0 ? group->polygons : k == 1 ? group->polylines : group->points;
            for (quint32 i = 0; i < count; i++)
            {
                quint32 type, attributeIdIndex, entries;
                qint32 zOrder;
                qint64 doubledArea;
                float detailScaleFactor;
                s >> type >> attributeIdIndex >> zOrder >> doubledArea >> detailScaleFactor >> entries;
                if (s.status() != QDataStream::Ok || entries > 1024)
                    return false;
                MapStyleEvaluationResult result(definitionsCount);
                for (quint32 e = 0; e < entries; e++)
                {
                    qint32 index;
                    QVariant value;
                    s >> index >> value;
                    if (index < 0 || index >= definitionIds.size())
                        return false;
                    result.setValue(definitionIds[index], value);
                }
                const std::shared_ptr<MapPrimitiviser::Primitive> primitive(new MapPrimitiviser::Primitive(
                    group, static_cast<MapPrimitiviser::PrimitiveType>(type), attributeIdIndex, result, detailScaleFactor));
                primitive->zOrder = zOrder;
                primitive->doubledArea = doubledArea;
                list.push_back(primitive);
            }
        }
        groups.push_back(group);
        prim->primitivesGroups.push_back(group);
    }
    const auto resolve =
        [&groups]
        (const quint64 ref) -> std::shared_ptr<const MapPrimitiviser::Primitive>
        {
            const auto gi = static_cast<int>(ref >> 34);
            const auto k = static_cast<int>((ref >> 32) & 3);
            const auto i = static_cast<int>(ref & 0xFFFFFFFFull);
            if (gi >= groups.size() || k > 2)
                return nullptr;
            const auto& list = k == 0 ? groups[gi]->polygons : k == 1 ? groups[gi]->polylines : groups[gi]->points;
            return i < list.size() ? list[i] : nullptr;
        };

    quint32 symbolsGroupCount;
    s >> symbolsGroupCount;
    if (s.status() != QDataStream::Ok || symbolsGroupCount > objectCount)
        return false;
    for (quint32 gi = 0; gi < symbolsGroupCount; gi++)
    {
        quint32 keyIndex, sourceIndex, count;
        bool canBeShownWithoutIcon;
        s >> keyIndex >> sourceIndex >> canBeShownWithoutIcon >> count;
        if (s.status() != QDataStream::Ok || keyIndex >= objectCount || sourceIndex >= objectCount || count > (1u << 16))
            return false;
        const std::shared_ptr<MapPrimitiviser::SymbolsGroup> group(
            new MapPrimitiviser::SymbolsGroup(objects[sourceIndex], canBeShownWithoutIcon));
        for (quint32 i = 0; i < count; i++)
        {
            qint32 kind, x, y, order;
            quint64 ref;
            s >> kind >> ref;
            const auto primitive = resolve(ref);
            if (!primitive)
                return false;
            std::shared_ptr<MapPrimitiviser::Symbol> symbol;
            std::shared_ptr<MapPrimitiviser::TextSymbol> text;
            std::shared_ptr<MapPrimitiviser::IconSymbol> icon;
            if (kind == 1)
                symbol = text = std::shared_ptr<MapPrimitiviser::TextSymbol>(new MapPrimitiviser::TextSymbol(primitive));
            else if (kind == 2)
                symbol = icon = std::shared_ptr<MapPrimitiviser::IconSymbol>(new MapPrimitiviser::IconSymbol(primitive));
            else
                return false;
            s >> x >> y >> order >> symbol->drawAlongPath >> symbol->intersectsWith >> symbol->intersectionSizeFactor
              >> symbol->intersectionSize >> symbol->intersectionMargin >> symbol->minDistance >> symbol->scaleFactor
              >> symbol->ignoreClick;
            symbol->location31 = PointI(x, y);
            symbol->order = order;
            if (text)
            {
                qint32 languageId, placement, verticalOffset, size, shadowRadius, wrapWidth;
                quint32 color, shadowColor, additionalCount;
                s >> text->baseValue >> text->value >> languageId >> text->drawOnPath >> text->topSide >> placement
                  >> additionalCount;
                if (additionalCount > 16)
                    return false;
                for (quint32 a = 0; a < additionalCount; a++)
                {
                    qint32 additional;
                    s >> additional;
                    text->additionalPlacements.push_back(static_cast<MapPrimitiviser::TextSymbol::Placement>(additional));
                }
                s >> verticalOffset >> color >> size >> shadowRadius >> shadowColor >> wrapWidth
                  >> text->isBold >> text->isItalic >> text->shieldResourceName >> text->underlayIconResourceNames;
                text->languageId = static_cast<LanguageId>(languageId);
                text->placement = static_cast<MapPrimitiviser::TextSymbol::Placement>(placement);
                text->verticalOffset = verticalOffset;
                text->color.argb = color;
                text->size = size;
                text->shadowRadius = shadowRadius;
                text->shadowColor.argb = shadowColor;
                text->wrapWidth = wrapWidth;
            }
            else
            {
                s >> icon->resourceName >> icon->underlayResourceNames >> icon->overlayResourceNames
                  >> icon->offsetFactor.x >> icon->offsetFactor.y >> icon->shieldResourceName;
            }
            group->symbols.push_back(symbol);
        }
        prim->symbolsGroups.insert(objects[keyIndex], group);
    }
    if (s.status() != QDataStream::Ok)
        return false;

    outMapObjects.clear();
    for (const auto& object : objects)
        outMapObjects.push_back(object);
    outSurfaceType = static_cast<MapSurfaceType>(surface);
    outPrimitivised = prim;
    return true;
}
