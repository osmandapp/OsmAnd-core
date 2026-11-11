#include "SymbolRasterizer_P.h"
#include "SymbolRasterizer.h"

#include "QtCommon.h"
#include <QReadWriteLock>
#include <QTextStream>

#include "ignore_warnings_on_external_includes.h"
#include <SkColorFilter.h>
#include "restore_internal_warnings.h"

#include "Nullable.h"
#include "MapPresentationEnvironment.h"
#include "MapStyleEvaluationResult.h"
#include "MapPrimitiviser.h"
#include "TextRasterizer.h"
#include "MapObject.h"
#include "ObfMapSectionInfo.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Stopwatch.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "Logging.h"

#include <string>
#include <cstring>
#include <QMutex>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace OsmAnd
{
    // zoom-bench E: zoom of the symbols request being rasterized on this thread
    int& zbRasterZoomRef()
    {
        static thread_local int zoom = -1;
        return zoom;
    }
}

namespace
{
    bool zbFlagE()
    {
        static const bool enabled = []() -> bool
        {
            char buffer[96] = { 0 };
#if defined(__ANDROID__)
            __system_property_get("debug.osmand.zb", buffer);
            if (!buffer[0])
                memcpy(buffer, "BCDEGLM", 8);  // zoom-bench: default variants on Android, "-" turns all off
#endif
            return std::string(buffer).find('E') != std::string::npos;
        }();
        return enabled;
    }

    bool zbFlagD()
    {
        static const bool enabled = []() -> bool
        {
            char buffer[96] = { 0 };
#if defined(__ANDROID__)
            __system_property_get("debug.osmand.zb", buffer);
            if (!buffer[0])
                memcpy(buffer, "BCDEGLM", 8);  // zoom-bench: default variants on Android, "-" turns all off
#endif
            return std::string(buffer).find('D') != std::string::npos;
        }();
        return enabled;
    }

    struct ZbTextValue
    {
        sk_sp<SkImage> image;
        QVector<SkScalar> glyphsWidth;
        float extraTop = 0, extraBottom = 0, lineSpacing = 0, fontAscent = 0;
    };
    struct ZbTextCache
    {
        QMutex mutex;
        QHash<QString, ZbTextValue> entries;
        qint64 pixels = 0;
        quint64 hits = 0, misses = 0;
    };
    ZbTextCache& zbTextCache()
    {
        static ZbTextCache cache;
        return cache;
    }
}


// #define OSMAND_DUMP_SYMBOLS 1
#if !defined(OSMAND_DUMP_SYMBOLS)
#   define OSMAND_DUMP_SYMBOLS 0
#endif // !defined(OSMAND_DUMP_SYMBOLS)

OsmAnd::SymbolRasterizer_P::SymbolRasterizer_P(SymbolRasterizer* const owner_)
    : owner(owner_)
{
}

OsmAnd::SymbolRasterizer_P::~SymbolRasterizer_P()
{
}

void OsmAnd::SymbolRasterizer_P::rasterize(
    const MapPrimitiviser::SymbolsGroupsCollection& symbolsGroups,
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    const FilterBySymbolsGroup filter,
    const std::shared_ptr<const IQueryController>& queryController) const
{
    // zoom-bench E: choose the captions worth rasterizing
    QSet<const void*> zbKeepText;
    const bool zbUseKeep = zbFlagE() && OsmAnd::zbRasterZoomRef() >= 0;
    if (zbUseKeep)
    {
        const int shift = qMax(0, 31 - OsmAnd::zbRasterZoomRef() - 3);
        QHash<quint64, QPair<int, const void*>> bestInCell;
        QHash<QString, QVector<QPair<int, const void*>>> onPathByText;
        for (const auto& entry : rangeOf(constOf(symbolsGroups)))
        {
            for (const auto& symbol : constOf(entry.value()->symbols))
            {
                const auto textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol);
                if (!textSymbol)
                    continue;
                if (!textSymbol->drawOnPath)
                {
                    const quint64 cell = (static_cast<quint64>(static_cast<quint32>(textSymbol->location31.x >> shift)) << 32)
                        | static_cast<quint32>(textSymbol->location31.y >> shift);
                    const auto it = bestInCell.find(cell);
                    if (it == bestInCell.end() || textSymbol->order < it->first)
                        bestInCell[cell] = qMakePair(textSymbol->order, static_cast<const void*>(textSymbol.get()));
                }
                else
                    onPathByText[textSymbol->value].push_back(qMakePair(textSymbol->order, static_cast<const void*>(textSymbol.get())));
            }
        }
        for (const auto& v : constOf(bestInCell))
            zbKeepText.insert(v.second);
        for (auto it = onPathByText.begin(); it != onPathByText.end(); ++it)
        {
            auto& list = it.value();
            std::sort(list.begin(), list.end(),
                [](const QPair<int, const void*>& l, const QPair<int, const void*>& r) { return l.first < r.first; });
            for (int i = 0; i < list.size() && i < 2; i++)
                zbKeepText.insert(list[i].second);
        }
    }

    for (const auto& symbolGroupEntry : rangeOf(constOf(symbolsGroups)))
    {
        if (queryController && queryController->isAborted())
            return;

        const auto& mapObject = symbolGroupEntry.key();
        const auto& symbolsGroup = symbolGroupEntry.value();

        //////////////////////////////////////////////////////////////////////////
        //if (mapObject->toString().contains("1333827773"))
        //{
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        // Apply filter, if it's present
        if (filter && !filter(symbolsGroup))
            continue;

        // Create group
        const std::shared_ptr<RasterizedSymbolsGroup> group(new RasterizedSymbolsGroup(
            mapObject, symbolsGroup->canBeShownWithoutIcon));

        // Minimum and maximum offsets to allow column of symbols grow in both directions without overlapping
        // These offsets are computed only in case symbol is not on-path and not along-path
        int32_t topOffset = 0;
        int32_t bottomOffset = 0;

        bool iconOnBottom = false;
        bool iconOnTop = false;

        bool bottomTextSectionTaken = false;
        bool hasIcon = false;
        for (const auto& symbol : symbolsGroup->symbols)
        {
            if (hasIcon && bottomTextSectionTaken)
                break;

            if (const auto iconSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::IconSymbol>(symbol))
                hasIcon = true;

            if (const auto textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol))
            {
                if (textSymbol->placement != TextSymbolPlacement::Top)
                    bottomTextSectionTaken = true;
            }
        }

        for (const auto& symbol : constOf(symbolsGroup->symbols))
        {
            if (queryController && queryController->isAborted())
                return;

            if (const auto& textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol))
            {
                if (zbUseKeep && !zbKeepText.contains(textSymbol.get()))
                    continue;
                TextRasterizer::Style style;
                if (!textSymbol->drawOnPath && textSymbol->shieldResourceName.isEmpty())
                    style.wrapWidth = textSymbol->wrapWidth;

                QList< sk_sp<const SkImage> > backgroundLayers;
                QString captionByBackground;
                if (!textSymbol->shieldResourceName.isEmpty())
                {
                    sk_sp<const SkImage> shield;
                    env->obtainIcon(textSymbol->shieldResourceName, textSymbol->scaleFactor, shield);

                    if (shield)
                    {
                        backgroundLayers.push_back(shield);
                        captionByBackground.append(textSymbol->shieldResourceName);
                    }
                }

                for (const auto& underlayIconResourceName : textSymbol->underlayIconResourceNames)
                {
                    sk_sp<const SkImage> icon;
                    if (env->obtainIcon(underlayIconResourceName, textSymbol->scaleFactor, icon) && icon)
                    {
                        backgroundLayers.push_back(icon);
                        if (!captionByBackground.isEmpty())
                            captionByBackground.append(QStringLiteral("_"));
                        captionByBackground.append(underlayIconResourceName);
                    }
                }

                sk_sp<const SkImage> backgroundImage;
                if (backgroundLayers.size() == 1)
                    backgroundImage = backgroundLayers.first();
                else
                    backgroundImage = SkiaUtilities::mergeImages(backgroundLayers);
                style
                    .setBackgroundImage(backgroundImage)
                    .setBold(textSymbol->isBold)
                    .setItalic(textSymbol->isItalic)
                    .setColor(textSymbol->color)
                    .setSize(static_cast<int>(textSymbol->size));

                if (textSymbol->shadowRadius > 0)
                {
                    style
                        .setHaloColor(textSymbol->shadowColor)
                        .setHaloRadius(textSymbol->shadowRadius);
                }

                float lineSpacing;
                float fontAscent;
                float symbolExtraTopSpace;
                float symbolExtraBottomSpace;
                QVector<SkScalar> glyphsWidth;
                sk_sp<SkImage> rasterizedText;
                QString zbKey;
                bool zbHit = false;
                if (zbFlagD())
                {
                    zbKey = QString::asprintf("%u|%d|%d|%u|%g|%u|%u|%p|%d|",
                        style.wrapWidth, style.bold ? 1 : 0, style.italic ? 1 : 0, style.color.argb, (double)style.size,
                        style.haloColor.argb, style.haloRadius, (const void*)style.backgroundImage.get(),
                        textSymbol->drawOnPath ? 1 : 0) + textSymbol->value;
                    auto& cache = zbTextCache();
                    QMutexLocker locker(&cache.mutex);
                    const auto it = cache.entries.constFind(zbKey);
                    if (it != cache.entries.cend())
                    {
                        rasterizedText = it->image;
                        glyphsWidth = it->glyphsWidth;
                        symbolExtraTopSpace = it->extraTop;
                        symbolExtraBottomSpace = it->extraBottom;
                        lineSpacing = it->lineSpacing;
                        fontAscent = it->fontAscent;
                        zbHit = true;
                        cache.hits++;
                    }
                    else
                        cache.misses++;
                    if (((cache.hits + cache.misses) % 2000) == 0)
                        LogPrintf(LogSeverityLevel::Info, ">>>> 0 TEXTCACHE 0.0: hits=%llu misses=%llu entries=%d MBpx=%.1f",
                            cache.hits, cache.misses, cache.entries.size(), cache.pixels * 4.0 / 1048576.0);
                }
                if (!zbHit)
                {
                    rasterizedText = owner->textRasterizer->rasterize(
                        textSymbol->value,
                        style,
                        textSymbol->drawOnPath ? &glyphsWidth : nullptr,
                        &symbolExtraTopSpace,
                        &symbolExtraBottomSpace,
                        &lineSpacing,
                        &fontAscent);
                    if (zbFlagD() && rasterizedText)
                    {
                        auto& cache = zbTextCache();
                        QMutexLocker locker(&cache.mutex);
                        if (cache.pixels > (16ll << 20))   // 16M pixels = 64 MB
                        {
                            cache.entries.clear();
                            cache.pixels = 0;
                        }
                        ZbTextValue v;
                        v.image = rasterizedText;
                        v.glyphsWidth = glyphsWidth;
                        v.extraTop = symbolExtraTopSpace;
                        v.extraBottom = symbolExtraBottomSpace;
                        v.lineSpacing = lineSpacing;
                        v.fontAscent = fontAscent;
                        cache.entries.insert(zbKey, v);
                        cache.pixels += static_cast<qint64>(rasterizedText->width()) * rasterizedText->height();
                    }
                }
                if (!rasterizedText)
                    continue;

#if OSMAND_DUMP_SYMBOLS
                {
                    QDir::current().mkpath("text_symbols");

                    QFile imageFile(QString::asprintf("text_symbols/%p.png", rasterizedText.get()));
                    if (imageFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        const auto imageData = rasterizedText->encodeToData(SkEncodedImageFormat::kPNG, 100);
                        imageFile.write(reinterpret_cast<const char*>(imageData->bytes()), imageData->size());
                        imageFile.close();
                    }

                    QFile textFile(QString::asprintf("text_symbols/%p.txt", qPrintable(QDir::currentPath())));
                    if (textFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        QTextStream(&textFile) << textSymbol->value;
                        textFile.close();
                    }
                }
#endif // OSMAND_DUMP_SYMBOLS

                if (textSymbol->drawOnPath)
                {
                    // Publish new rasterized symbol
                    const std::shared_ptr<RasterizedOnPathSymbol> rasterizedSymbol(new RasterizedOnPathSymbol(
                        group,
                        textSymbol));
                    rasterizedSymbol->image = qMove(rasterizedText);
                    rasterizedSymbol->order = textSymbol->order;
                    rasterizedSymbol->contentType = RasterizedSymbol::ContentType::Text;
                    rasterizedSymbol->content = textSymbol->baseValue;
                    rasterizedSymbol->languageId = textSymbol->languageId;
                    rasterizedSymbol->minDistance = textSymbol->minDistance;
                    rasterizedSymbol->ignoreClick = textSymbol->ignoreClick;
                    rasterizedSymbol->glyphsWidth = glyphsWidth;
                    group->symbols.push_back(qMove(rasterizedSymbol));
                }
                else
                {
                    // Calculate offset. Since offset specifies center, it's a sum of
                    //  - top/bottom offset
                    //  - extra top/bottom space (which should be in texture, but not rendered, since transparent)
                    //  - height / 2
                    PointI offset;
                    Nullable<PointI> additionalOffset;

                    // If bottom text section is free, move top section text to bottom and reset vertical offset
                    bool drawInTopSection = textSymbol->placement == TextSymbolPlacement::Top;
                    int verticalOffset = textSymbol->verticalOffset;
                    if (drawInTopSection && !bottomTextSectionTaken)
                    {
                        drawInTopSection = false;
                        verticalOffset = 0;
                    }

                    const bool hasAdditionalTopSection = textSymbol->placement != TextSymbolPlacement::Top
                        && std::any_of(textSymbol->additionalPlacements,
                            []
                            (const TextSymbolPlacement additionalPlacement) -> bool
                            {
                                return additionalPlacement == TextSymbolPlacement::Top;
                            });

                    offset.y += verticalOffset;

                    if (!group->symbols.isEmpty() && !textSymbol->drawAlongPath)
                    {
                        const auto halfHeight = rasterizedText->height() / 2;
                        if (drawInTopSection)
                        {
                            offset.y += topOffset;
                            offset.y -= symbolExtraBottomSpace;
                            offset.y -= halfHeight;

                            iconOnTop = false;
                        }
                        else
                        {
                            offset.y += bottomOffset;
                            offset.y += symbolExtraTopSpace;
                            offset.y += halfHeight;
                            if (iconOnBottom && qFuzzyIsNull(symbolExtraTopSpace))
                                offset.y += qCeil(-fontAscent / 2.0f);
                            iconOnBottom = false;

                            if (hasAdditionalTopSection && hasIcon)
                            {
                                additionalOffset = PointI();
                                additionalOffset->y += topOffset;
                                if (iconOnTop)
                                    additionalOffset->y += qFloor(fontAscent / 2.0f);
                                iconOnTop = false;
                            }
                        }
                    }

                    // Publish new rasterized symbol
                    const std::shared_ptr<RasterizedSpriteSymbol> rasterizedSymbol(new RasterizedSpriteSymbol(group, textSymbol));
                    rasterizedSymbol->image = rasterizedText;
                    rasterizedSymbol->order = textSymbol->order;
                    rasterizedSymbol->contentType = RasterizedSymbol::ContentType::Text;
                    rasterizedSymbol->content = textSymbol->baseValue != QStringViewLiteral(".")
                        || captionByBackground.isEmpty() ? textSymbol->baseValue : captionByBackground;
                    rasterizedSymbol->languageId = textSymbol->languageId;
                    rasterizedSymbol->minDistance = textSymbol->minDistance;
                    rasterizedSymbol->ignoreClick = textSymbol->ignoreClick;
                    rasterizedSymbol->location31 = textSymbol->location31;
                    rasterizedSymbol->offset = textSymbol->drawAlongPath ? PointI() : offset;
                    rasterizedSymbol->additionalOffset = textSymbol->drawAlongPath ? Nullable<PointI>() : additionalOffset;
                    rasterizedSymbol->drawAlongPath = textSymbol->drawAlongPath;
                    if (!qIsNaN(textSymbol->intersectionSizeFactor) && !qFuzzyCompare(textSymbol->size, 0.0f))
                    {
                        const auto margin = static_cast<int>((textSymbol->intersectionSizeFactor - 1.0f) * textSymbol->size);
                        const PointI size(rasterizedText->width() + margin, rasterizedText->height() + margin);
                        rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), size);
                    }
                    else if (!qIsNaN(textSymbol->intersectionSize))
                    {
                        rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                            static_cast<int>(textSymbol->intersectionSize),
                            static_cast<int>(textSymbol->intersectionSize)));
                    }
                    else if (!qIsNaN(textSymbol->intersectionMargin))
                    {
                        rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                            rasterizedText->width() + static_cast<int>(textSymbol->intersectionMargin),
                            rasterizedText->height() + static_cast<int>(textSymbol->intersectionMargin)));
                    }
                    else
                    {
                        rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                            static_cast<int>(rasterizedText->width()),
                            static_cast<int>(rasterizedText->height())));

                        rasterizedSymbol->intersectionBBox.top() -= static_cast<int>(symbolExtraTopSpace);
                        rasterizedSymbol->intersectionBBox.bottom() += static_cast<int>(symbolExtraBottomSpace);
                    }
                    group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

                    // Next symbol should also take into account:
                    //  - height / 2
                    //  - extra top/bottom space (which should be in texture, but not rendered, since transparent)
                    //  - spacing between lines
                    if (!textSymbol->drawAlongPath)
                    {
                        const auto halfHeight = rasterizedText->height() / 2;

                        auto currentTopOffset = offset.y;
                        auto currentBottomOffset = offset.y;
                        if (additionalOffset)
                        {
                            currentTopOffset = std::min(currentTopOffset, additionalOffset->y);
                            currentBottomOffset = std::max(currentBottomOffset, additionalOffset->y);
                        }

                        const auto topAdvance = -qCeil(halfHeight + symbolExtraTopSpace + lineSpacing + verticalOffset);
                        topOffset = std::min(topOffset, currentTopOffset + topAdvance);

                        const auto bottomAdvance = qCeil(halfHeight + symbolExtraBottomSpace + lineSpacing - verticalOffset);
                        bottomOffset = std::max(bottomOffset, currentBottomOffset + bottomAdvance);
                    }
                }
            }
            else if (const auto& iconSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::IconSymbol>(symbol))
            {
                sk_sp<const SkImage> icon;
                if (!env->obtainIcon(iconSymbol->resourceName, iconSymbol->scaleFactor, icon) || !icon)
                    continue;

                QList< sk_sp<const SkImage> > layers;

                if (!iconSymbol->shieldResourceName.isEmpty())
                {
                    sk_sp<const SkImage> background;
                    env->obtainIcon(iconSymbol->shieldResourceName, iconSymbol->scaleFactor, background);
                    if (background)
                        layers.push_back(background);
                }

                for (const auto& overlayResourceName : constOf(iconSymbol->underlayResourceNames))
                {
                    sk_sp<const SkImage> underlay;
                    if (!env->obtainIcon(overlayResourceName, iconSymbol->scaleFactor, underlay) || !underlay)
                        continue;

                    layers.push_back(underlay);
                }
                layers.push_back(icon);
                for (const auto& overlayResourceName : constOf(iconSymbol->overlayResourceNames))
                {
                    sk_sp<const SkImage> overlay;
                    if (!env->obtainIcon(overlayResourceName, iconSymbol->scaleFactor, overlay) || !overlay)
                        continue;

                    layers.push_back(overlay);
                }

                // Compose final image
                sk_sp<const SkImage> rasterizedIcon;
                if (layers.size() == 1)
                    rasterizedIcon = layers.first();
                else
                    rasterizedIcon = SkiaUtilities::mergeImages(layers);

#if OSMAND_DUMP_SYMBOLS
                {
                    QDir::current().mkpath("icon_symbols");

                    QFile imageFile(QString::asprintf("icon_symbols/%p.png", rasterizedIcon.get()));
                    if (imageFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        const auto imageData = rasterizedIcon->encodeToData(SkEncodedImageFormat::kPNG, 100);
                        imageFile.write(reinterpret_cast<const char*>(imageData->bytes()), imageData->size());
                        imageFile.close();
                    }
                }
#endif // OSMAND_DUMP_SYMBOLS

                // Icons offset always measured from center, without indents from other symbols
                PointI symbolOffset;
                PointI iconOffset;
                if (!qFuzzyIsNull(iconSymbol->offsetFactor.x))
                {
                    symbolOffset.x = qRound(iconSymbol->offsetFactor.x * icon->width());
                    iconOffset.x = qRound(iconSymbol->offsetFactor.x * rasterizedIcon->width());
                }
                if (!qFuzzyIsNull(iconSymbol->offsetFactor.y))
                {
                    symbolOffset.y = qRound(iconSymbol->offsetFactor.y * icon->height());
                    iconOffset.y = qRound(iconSymbol->offsetFactor.y * rasterizedIcon->height());
                }

                // Publish new rasterized symbol
                const std::shared_ptr<RasterizedSpriteSymbol> rasterizedSymbol(new RasterizedSpriteSymbol(group, iconSymbol));
                rasterizedSymbol->image = rasterizedIcon;
                rasterizedSymbol->order = iconSymbol->order;
                rasterizedSymbol->contentType = RasterizedSymbol::ContentType::Icon;
                rasterizedSymbol->content = iconSymbol->resourceName;
                rasterizedSymbol->languageId = LanguageId::Invariant;
                rasterizedSymbol->minDistance = iconSymbol->minDistance;
                rasterizedSymbol->ignoreClick = iconSymbol->ignoreClick;
                rasterizedSymbol->location31 = iconSymbol->location31;
                rasterizedSymbol->offset = symbolOffset;
                rasterizedSymbol->drawAlongPath = iconSymbol->drawAlongPath;
                if (!qIsNaN(iconSymbol->intersectionSizeFactor))
                {
                    rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                        static_cast<int>(iconSymbol->intersectionSizeFactor * rasterizedIcon->width()),
                        static_cast<int>(iconSymbol->intersectionSizeFactor * rasterizedIcon->height())));
                }
                else if (!qIsNaN(iconSymbol->intersectionSize))
                {
                    rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                        static_cast<int>(iconSymbol->intersectionSize * iconSymbol->scaleFactor),
                        static_cast<int>(iconSymbol->intersectionSize * iconSymbol->scaleFactor)));
                }
                else if (!qIsNaN(iconSymbol->intersectionMargin))
                {
                    rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                        rasterizedIcon->width() + static_cast<int>(iconSymbol->intersectionMargin),
                        rasterizedIcon->height() + static_cast<int>(iconSymbol->intersectionMargin)));
                }
                else
                {
                    rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                        static_cast<int>(rasterizedIcon->width()),
                        static_cast<int>(rasterizedIcon->height())));
                }
                group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

                // Next symbol should also take into account:
                //  - height / 2
                if (!iconSymbol->drawAlongPath)
                {
                    const auto halfHeight = rasterizedIcon->height() / 2;

                    if (iconOffset.y - halfHeight < topOffset)
                    {
                        topOffset = iconOffset.y - halfHeight;
                        iconOnTop = true;
                    }

                    if (iconOffset.y + halfHeight > bottomOffset)
                    {
                        bottomOffset = iconOffset.y + halfHeight;
                        iconOnBottom = true;
                    }
                }
            }
        }

        // Add group to output
        outSymbolsGroups.push_back(qMove(group));
    }
}
