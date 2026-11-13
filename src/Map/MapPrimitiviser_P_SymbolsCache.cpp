#include "MapPrimitiviser_P.h"
#include "MapPrimitiviser.h"

#include "ignore_warnings_on_external_includes.h"
#include <QDataStream>
#include <QSet>
#include "restore_internal_warnings.h"

#include <algorithm>
#include <cstring>

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
// All symbols groups of the tile are kept, as the primitiviser made them (labels are already thinned by the
// OBF reader); the renderer places them as for a tile that was not cached.
// Objects keep their ids, tags and bbox (a tap finds the object, its name comes from the drawn caption);
// geometry is kept only for objects with text on a path, which the renderer lays out along it.
// Tried and dropped: one caption per 1/16-tile cell (-29% bytes, ~1 ms faster restore per tile, but captions
// such as station names went missing) and dropping groups overlapped by a lower-order group (-1%).

namespace
{
    const quint32 kMagic = 0x4F534D53; // 'OSMS'

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

    typedef OsmAnd::MapPrimitiviser MP;
    typedef MP::Symbol Symbol;
    typedef MP::TextSymbol TextSymbol;
    typedef MP::IconSymbol IconSymbol;

    // three primitive lists of a group, in the order they are written
    inline const MP::PrimitivesCollection& primitivesList(const MP::PrimitivesGroup& group, const int k)
    {
        return k == 0 ? group.polygons : k == 1 ? group.polylines : group.points;
    }

    inline MP::PrimitivesCollection& primitivesList(MP::PrimitivesGroup& group, const int k)
    {
        return k == 0 ? group.polygons : k == 1 ? group.polylines : group.points;
    }

    struct KeptGroups
    {
        QSet<const OsmAnd::MapObject*> groups;      // symbols groups, by key object
        QSet<const OsmAnd::MapObject*> objects;     // key and source objects of kept groups
        QSet<const OsmAnd::MapObject*> pathObjects; // objects whose geometry is needed
    };

    // Step 1: groups with symbols; text on a path needs its object's geometry
    KeptGroups selectGroups(const MP::PrimitivisedObjects& prim)
    {
        KeptGroups result;
        for (auto it = prim.symbolsGroups.cbegin(); it != prim.symbolsGroups.cend(); ++it)
        {
            const auto& symbols = it.value()->symbols;
            if (symbols.isEmpty())
                continue;
            const bool onPath = std::any_of(symbols.cbegin(), symbols.cend(),
                []
                (const std::shared_ptr<const Symbol>& symbol) -> bool
                {
                    const auto text = std::dynamic_pointer_cast<const TextSymbol>(symbol);
                    return symbol->drawAlongPath || (text && text->drawOnPath);
                });
            const auto key = it.key().get();
            const auto source = it.value()->sourceObject.get();
            result.groups.insert(key);
            result.objects.insert(key);
            result.objects.insert(source);
            if (onPath)
            {
                result.pathObjects.insert(key);
                result.pathObjects.insert(source);
            }
        }
        return result;
    }

    // Step 2: what gets written
    struct SymbolsRecord
    {
        MP::PrimitivesGroupsCollection groups;
        QList< std::shared_ptr<const OsmAnd::MapObject> > objects;
        QHash<const OsmAnd::MapObject*, quint32> objectIndex;
        // primitive -> group index << 34 | list << 32 | index in the list
        QHash<const MP::Primitive*, quint64> primitiveRef;

        void addObject(const std::shared_ptr<const OsmAnd::MapObject>& object)
        {
            if (objectIndex.contains(object.get()))
                return;
            objectIndex.insert(object.get(), objects.size());
            objects.push_back(object);
        }
    };

    SymbolsRecord collectRecord(const MP::PrimitivisedObjects& prim, const KeptGroups& kept)
    {
        SymbolsRecord record;
        for (const auto& group : prim.primitivesGroups)
            if (kept.objects.contains(group->sourceObject.get()))
                record.groups.push_back(group);
        for (const auto& group : record.groups)
            record.addObject(group->sourceObject);
        for (auto it = prim.symbolsGroups.cbegin(); it != prim.symbolsGroups.cend(); ++it)
        {
            if (!kept.groups.contains(it.key().get()))
                continue;
            record.addObject(it.key());
            record.addObject(it.value()->sourceObject);
        }
        for (int gi = 0; gi < record.groups.size(); gi++)
        {
            for (quint64 k = 0; k < 3; k++)
            {
                const auto& list = primitivesList(*record.groups[gi], k);
                for (int i = 0; i < list.size(); i++)
                    record.primitiveRef.insert(list[i].get(), (static_cast<quint64>(gi) << 34) | (k << 32) | static_cast<quint64>(i));
            }
        }
        return record;
    }

    bool writeObjects(QDataStream& s, const SymbolsRecord& record, const KeptGroups& kept)
    {
        QStringList sectionKeys;
        QHash<const OsmAnd::ObfMapSectionInfo*, qint32> sectionIndex;
        for (const auto& object : record.objects)
        {
            const auto binary = std::dynamic_pointer_cast<const OsmAnd::BinaryMapObject>(object);
            if (binary && !sectionIndex.contains(binary->section.get()))
            {
                sectionIndex.insert(binary->section.get(), sectionKeys.size());
                sectionKeys.push_back(OsmAnd::MapPrimitiviser_P::getDiskCacheSectionKey(*binary->section));
            }
        }
        s << sectionKeys;
        s << static_cast<quint32>(record.objects.size());
        for (const auto& object : record.objects)
        {
            const auto binary = std::dynamic_pointer_cast<const OsmAnd::BinaryMapObject>(object);
            qint32 kind = -1;
            if (binary)
                kind = 0;
            else if (std::dynamic_pointer_cast<const MP::SurfaceMapObject>(object))
                kind = 1;
            else if (std::dynamic_pointer_cast<const MP::CoastlineMapObject>(object))
                kind = 2;
            if (kind < 0)
                return false;  // an object that can not be restored: this tile is not cached
            s << kind;
            if (binary)
            {
                s << sectionIndex.value(binary->section.get())
                  << static_cast<qint32>(binary->level->minZoom) << static_cast<qint32>(binary->level->maxZoom)
                  << static_cast<quint64>(binary->id.id) << static_cast<quint64>(binary->blockId.id);
            }
            s << object->attributeIds << object->additionalAttributeIds << object->isArea << object->isCoastline;
            const bool withGeometry = kept.pathObjects.contains(object.get());
            writePoints(s, withGeometry ? object->points31 : QVector<OsmAnd::PointI>());
            s << static_cast<quint32>(withGeometry ? object->innerPolygonsPoints31.size() : 0);
            if (withGeometry)
                for (const auto& inner : object->innerPolygonsPoints31)
                    writePoints(s, inner);
            s << static_cast<qint32>(object->bbox31.top()) << static_cast<qint32>(object->bbox31.left())
              << static_cast<qint32>(object->bbox31.bottom()) << static_cast<qint32>(object->bbox31.right())
              << static_cast<qint32>(object->labelX) << static_cast<qint32>(object->labelY);
        }
        return true;
    }

    bool writePrimitives(QDataStream& s, const SymbolsRecord& record, const OsmAnd::IMapStyle& mapStyle)
    {
        // value definition ids are given out per process: names go first
        QHash<qint32, qint32> definitionIndex;
        QStringList definitionNames;
        for (const auto& group : record.groups)
        {
            for (int k = 0; k < 3; k++)
            {
                for (const auto& primitive : primitivesList(*group, k))
                {
                    for (const auto& entry : primitive->evaluationResult.entries)
                    {
                        if (definitionIndex.contains(entry.first))
                            continue;
                        const auto& definition = mapStyle.getValueDefinitionRefById(entry.first);
                        if (!definition)
                            return false;
                        definitionIndex.insert(entry.first, definitionNames.size());
                        definitionNames.push_back(definition->name);
                    }
                }
            }
        }
        s << definitionNames;
        s << static_cast<quint32>(record.groups.size());
        for (const auto& group : record.groups)
        {
            s << record.objectIndex.value(group->sourceObject.get());
            for (int k = 0; k < 3; k++)
            {
                const auto& list = primitivesList(*group, k);
                s << static_cast<quint32>(list.size());
                for (const auto& primitive : list)
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
        return true;
    }

    bool writeSymbol(QDataStream& s, const SymbolsRecord& record, const std::shared_ptr<const Symbol>& symbol)
    {
        const auto ref = record.primitiveRef.constFind(symbol->primitive.get());
        const auto text = std::dynamic_pointer_cast<const TextSymbol>(symbol);
        const auto icon = std::dynamic_pointer_cast<const IconSymbol>(symbol);
        if (ref == record.primitiveRef.cend() || (!text && !icon))
            return false;
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
        return true;
    }

    bool writeSymbolsGroups(QDataStream& s, const MP::PrimitivisedObjects& prim, const SymbolsRecord& record,
        const KeptGroups& kept)
    {
        s << static_cast<quint32>(kept.groups.size());
        for (auto it = prim.symbolsGroups.cbegin(); it != prim.symbolsGroups.cend(); ++it)
        {
            if (!kept.groups.contains(it.key().get()))
                continue;
            const auto& group = it.value();
            s << record.objectIndex.value(it.key().get()) << record.objectIndex.value(group->sourceObject.get())
              << group->canBeShownWithoutIcon;
            s << static_cast<quint32>(group->symbols.size());
            for (const auto& symbol : group->symbols)
                if (!writeSymbol(s, record, symbol))
                    return false;
        }
        return true;
    }

    // ---- reading ----

    bool readObject(QDataStream& s, const QVector< std::shared_ptr<const OsmAnd::ObfMapSectionInfo> >& sections,
        std::shared_ptr<OsmAnd::MapObject>& outObject)
    {
        qint32 kind;
        s >> kind;
        if (kind == 0)
        {
            qint32 sectionIndex, minZoom, maxZoom;
            quint64 id, blockId;
            s >> sectionIndex >> minZoom >> maxZoom >> id >> blockId;
            if (sectionIndex < 0 || sectionIndex >= sections.size())
                return false;
            const auto& section = sections[sectionIndex];
            std::shared_ptr<const OsmAnd::ObfMapSectionLevel> level;
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
            const auto binary = OsmAnd::MapPrimitiviser_P::newBinaryMapObject(section, level);
            binary->id.id = id;
            binary->blockId.id = blockId;
            outObject = binary;
        }
        else if (kind == 1)
            outObject.reset(new MP::SurfaceMapObject());
        else if (kind == 2)
            outObject.reset(new MP::CoastlineMapObject());
        else
            return false;
        auto& object = *outObject;
        s >> object.attributeIds >> object.additionalAttributeIds >> object.isArea >> object.isCoastline;
        if (!readPoints(s, object.points31))
            return false;
        quint32 innerCount;
        s >> innerCount;
        if (innerCount > (1u << 16))
            return false;
        for (quint32 i = 0; i < innerCount; i++)
        {
            QVector<OsmAnd::PointI> points;
            if (!readPoints(s, points))
                return false;
            object.innerPolygonsPoints31.push_back(qMove(points));
        }
        qint32 top, left, bottom, right, labelX, labelY;
        s >> top >> left >> bottom >> right >> labelX >> labelY;
        object.bbox31 = OsmAnd::AreaI(top, left, bottom, right);
        object.labelX = labelX;
        object.labelY = labelY;
        return s.status() == QDataStream::Ok;
    }

    std::shared_ptr<MP::Primitive> readPrimitive(QDataStream& s, const std::shared_ptr<MP::PrimitivesGroup>& group,
        const QVector<qint32>& definitionIds, const int definitionsCount)
    {
        quint32 type, attributeIdIndex, entries;
        qint32 zOrder;
        qint64 doubledArea;
        float detailScaleFactor;
        s >> type >> attributeIdIndex >> zOrder >> doubledArea >> detailScaleFactor >> entries;
        if (s.status() != QDataStream::Ok || entries > 1024)
            return nullptr;
        OsmAnd::MapStyleEvaluationResult result(definitionsCount);
        for (quint32 e = 0; e < entries; e++)
        {
            qint32 index;
            QVariant value;
            s >> index >> value;
            if (index < 0 || index >= definitionIds.size())
                return nullptr;
            result.setValue(definitionIds[index], value);
        }
        const auto primitive = OsmAnd::MapPrimitiviser_P::newPrimitive(
            group, static_cast<MP::PrimitiveType>(type), attributeIdIndex, result, detailScaleFactor);
        primitive->zOrder = zOrder;
        primitive->doubledArea = doubledArea;
        return primitive;
    }

    bool readPrimitivesGroups(QDataStream& s, const QVector< std::shared_ptr<OsmAnd::MapObject> >& objects,
        const OsmAnd::IMapStyle& mapStyle, MP::PrimitivisedObjects& prim,
        QVector< std::shared_ptr<MP::PrimitivesGroup> >& outGroups)
    {
        QStringList definitionNames;
        s >> definitionNames;
        QVector<qint32> definitionIds(definitionNames.size());
        for (int i = 0; i < definitionNames.size(); i++)
        {
            definitionIds[i] = mapStyle.getValueDefinitionIdByName(definitionNames[i]);
            if (definitionIds[i] < 0)
                return false;
        }
        const int definitionsCount = mapStyle.getValueDefinitionsCount();
        quint32 groupCount;
        s >> groupCount;
        if (s.status() != QDataStream::Ok || groupCount > static_cast<quint32>(objects.size()))
            return false;
        for (quint32 gi = 0; gi < groupCount; gi++)
        {
            quint32 objectIndex;
            s >> objectIndex;
            if (objectIndex >= static_cast<quint32>(objects.size()))
                return false;
            const auto group = OsmAnd::MapPrimitiviser_P::newPrimitivesGroup(objects[objectIndex]);
            for (int k = 0; k < 3; k++)
            {
                quint32 count;
                s >> count;
                if (s.status() != QDataStream::Ok || count > (1u << 16))
                    return false;
                for (quint32 i = 0; i < count; i++)
                {
                    const auto primitive = readPrimitive(s, group, definitionIds, definitionsCount);
                    if (!primitive)
                        return false;
                    primitivesList(*group, k).push_back(primitive);
                }
            }
            outGroups.push_back(group);
            prim.primitivesGroups.push_back(group);
        }
        return true;
    }

    void readText(QDataStream& s, TextSymbol& text, bool& outOk)
    {
        qint32 languageId, placement, verticalOffset, size, shadowRadius, wrapWidth;
        quint32 color, shadowColor, additionalCount;
        s >> text.baseValue >> text.value >> languageId >> text.drawOnPath >> text.topSide >> placement >> additionalCount;
        outOk = additionalCount <= 16;
        if (!outOk)
            return;
        for (quint32 a = 0; a < additionalCount; a++)
        {
            qint32 additional;
            s >> additional;
            text.additionalPlacements.push_back(static_cast<TextSymbol::Placement>(additional));
        }
        s >> verticalOffset >> color >> size >> shadowRadius >> shadowColor >> wrapWidth
          >> text.isBold >> text.isItalic >> text.shieldResourceName >> text.underlayIconResourceNames;
        text.languageId = static_cast<OsmAnd::LanguageId>(languageId);
        text.placement = static_cast<TextSymbol::Placement>(placement);
        text.verticalOffset = verticalOffset;
        text.color.argb = color;
        text.size = size;
        text.shadowRadius = shadowRadius;
        text.shadowColor.argb = shadowColor;
        text.wrapWidth = wrapWidth;
    }

    std::shared_ptr<Symbol> readSymbol(QDataStream& s, const QVector< std::shared_ptr<MP::PrimitivesGroup> >& groups)
    {
        qint32 kind, x, y, order;
        quint64 ref;
        s >> kind >> ref;
        const auto gi = static_cast<int>(ref >> 34);
        const auto k = static_cast<int>((ref >> 32) & 3);
        const auto i = static_cast<int>(ref & 0xFFFFFFFFull);
        if (gi >= groups.size() || k > 2 || i >= primitivesList(*groups[gi], k).size())
            return nullptr;
        const auto& primitive = primitivesList(*groups[gi], k)[i];

        std::shared_ptr<Symbol> symbol;
        std::shared_ptr<TextSymbol> text;
        std::shared_ptr<IconSymbol> icon;
        if (kind == 1)
            symbol = text = OsmAnd::MapPrimitiviser_P::newTextSymbol(primitive);
        else if (kind == 2)
            symbol = icon = OsmAnd::MapPrimitiviser_P::newIconSymbol(primitive);
        else
            return nullptr;
        s >> x >> y >> order >> symbol->drawAlongPath >> symbol->intersectsWith >> symbol->intersectionSizeFactor
          >> symbol->intersectionSize >> symbol->intersectionMargin >> symbol->minDistance >> symbol->scaleFactor
          >> symbol->ignoreClick;
        symbol->location31 = OsmAnd::PointI(x, y);
        symbol->order = order;
        if (text)
        {
            bool ok;
            readText(s, *text, ok);
            if (!ok)
                return nullptr;
        }
        else
        {
            s >> icon->resourceName >> icon->underlayResourceNames >> icon->overlayResourceNames
              >> icon->offsetFactor.x >> icon->offsetFactor.y >> icon->shieldResourceName;
        }
        return symbol;
    }

    bool readSymbolsGroups(QDataStream& s, const QVector< std::shared_ptr<OsmAnd::MapObject> >& objects,
        const QVector< std::shared_ptr<MP::PrimitivesGroup> >& groups, MP::PrimitivisedObjects& prim)
    {
        const auto objectCount = static_cast<quint32>(objects.size());
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
            const auto group = OsmAnd::MapPrimitiviser_P::newSymbolsGroup(objects[sourceIndex], canBeShownWithoutIcon);
            for (quint32 i = 0; i < count; i++)
            {
                const auto symbol = readSymbol(s, groups);
                if (!symbol)
                    return false;
                group->symbols.push_back(symbol);
            }
            prim.symbolsGroups.insert(objects[keyIndex], group);
        }
        return s.status() == QDataStream::Ok;
    }
}

std::shared_ptr<OsmAnd::BinaryMapObject> OsmAnd::MapPrimitiviser_P::newBinaryMapObject(
    const std::shared_ptr<const ObfMapSectionInfo>& section,
    const std::shared_ptr<const ObfMapSectionLevel>& level)
{
    return std::shared_ptr<BinaryMapObject>(new BinaryMapObject(section, level));
}

std::shared_ptr<OsmAnd::MapPrimitiviser::PrimitivesGroup> OsmAnd::MapPrimitiviser_P::newPrimitivesGroup(
    const std::shared_ptr<const MapObject>& sourceObject)
{
    return std::shared_ptr<MapPrimitiviser::PrimitivesGroup>(new MapPrimitiviser::PrimitivesGroup(sourceObject));
}

std::shared_ptr<OsmAnd::MapPrimitiviser::Primitive> OsmAnd::MapPrimitiviser_P::newPrimitive(
    const std::shared_ptr<const MapPrimitiviser::PrimitivesGroup>& group,
    const MapPrimitiviser::PrimitiveType type,
    const uint32_t attributeIdIndex,
    const MapStyleEvaluationResult& evaluationResult,
    const float detailScaleFactor)
{
    return std::shared_ptr<MapPrimitiviser::Primitive>(
        new MapPrimitiviser::Primitive(group, type, attributeIdIndex, evaluationResult, detailScaleFactor));
}

std::shared_ptr<OsmAnd::MapPrimitiviser::SymbolsGroup> OsmAnd::MapPrimitiviser_P::newSymbolsGroup(
    const std::shared_ptr<const MapObject>& sourceObject, const bool canBeShownWithoutIcon)
{
    return std::shared_ptr<MapPrimitiviser::SymbolsGroup>(
        new MapPrimitiviser::SymbolsGroup(sourceObject, canBeShownWithoutIcon));
}

std::shared_ptr<OsmAnd::MapPrimitiviser::TextSymbol> OsmAnd::MapPrimitiviser_P::newTextSymbol(
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive)
{
    return std::shared_ptr<MapPrimitiviser::TextSymbol>(new MapPrimitiviser::TextSymbol(primitive));
}

std::shared_ptr<OsmAnd::MapPrimitiviser::IconSymbol> OsmAnd::MapPrimitiviser_P::newIconSymbol(
    const std::shared_ptr<const MapPrimitiviser::Primitive>& primitive)
{
    return std::shared_ptr<MapPrimitiviser::IconSymbol>(new MapPrimitiviser::IconSymbol(primitive));
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
    const auto kept = selectGroups(*prim);
    const auto record = collectRecord(*prim, kept);

    QByteArray out;
    QDataStream s(&out, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_5_6);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);
    s << kMagic << static_cast<qint32>(prim->zoom) << static_cast<qint32>(surfaceType) << prim->coastlineMistake;
    writeDouble(s, prim->scaleDivisor31ToPixel.x);
    writeDouble(s, prim->scaleDivisor31ToPixel.y);
    if (!writeObjects(s, record, kept)
        || !writePrimitives(s, record, *prim->mapPresentationEnvironment->mapStyle)
        || !writeSymbolsGroups(s, *prim, record, kept)
        || s.status() != QDataStream::Ok)
    {
        return QByteArray();
    }
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
        objectSections[i] = sections.value(sectionKeys[i]);
        // the attribute mapping is read with the first map block: until a normal read happened, it is a miss
        if (!objectSections[i] || !objectSections[i]->getAttributeMapping())
            return false;
    }

    quint32 objectCount;
    s >> objectCount;
    if (s.status() != QDataStream::Ok || objectCount > (1u << 20))
        return false;
    QVector< std::shared_ptr<MapObject> > objects(objectCount);
    for (auto& object : objects)
        if (!readObject(s, objectSections, object))
            return false;

    const std::shared_ptr<MapPrimitiviser::PrimitivisedObjects> prim(new MapPrimitiviser::PrimitivisedObjects(
        env, nullptr, static_cast<ZoomLevel>(zoom), scaleDivisor));
    prim->coastlineMistake = coastlineMistake;
    QVector< std::shared_ptr<MapPrimitiviser::PrimitivesGroup> > groups;
    if (!readPrimitivesGroups(s, objects, *env->mapStyle, *prim, groups)
        || !readSymbolsGroups(s, objects, groups, *prim))
    {
        return false;
    }

    outMapObjects.clear();
    for (const auto& object : objects)
        outMapObjects.push_back(object);
    outSurfaceType = static_cast<MapSurfaceType>(surface);
    outPrimitivised = prim;
    return true;
}
