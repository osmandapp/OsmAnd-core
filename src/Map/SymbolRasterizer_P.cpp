#include "SymbolRasterizer_P.h"
#include "SymbolRasterizer.h"

#include "QtCommon.h"
#include <QReadWriteLock>
#include <QTextStream>

#include "ignore_warnings_on_external_includes.h"
#include <SkColorFilter.h>
#include "restore_internal_warnings.h"

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
#include "QuadTree.h"

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
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    const FilterByMapObject filter,
    const std::shared_ptr<const IQueryController>& queryController) const
{
    const auto& env = primitivisedObjects->mapPresentationEnvironment;
    
    QList<std::multimap<int, std::shared_ptr<const RasterizedSymbol>>> symbolsContainer;
    std::map<std::shared_ptr<const RasterizedSymbol>, const std::shared_ptr<RasterizedSymbolsGroup>> mapGroups;
    for (const auto& symbolGroupEntry : rangeOf(constOf(primitivisedObjects->symbolsGroups)))
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
        if (filter && !filter(mapObject))
            continue;

        // Create group
        const std::shared_ptr<RasterizedSymbolsGroup> group(new RasterizedSymbolsGroup(
            mapObject));
        std::multimap<int, std::shared_ptr<const RasterizedSymbol>> orderMapSymbols;
        bool isSymbolsAdded = false;

        // Total offset allows several symbols to stack into column. Offset specifies center of symbol bitmap.
        // This offset is computed only in case symbol is not on-path and not along-path
        PointI totalOffset;
        bool textAfterIcon = false;

        for (const auto& symbol : constOf(symbolsGroup->symbols))
        {
            if (queryController && queryController->isAborted())
                return;

            if (const auto& textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol))
            {
                TextRasterizer::Style style;
                if (!textSymbol->drawOnPath && textSymbol->shieldResourceName.isEmpty())
                    style.wrapWidth = textSymbol->wrapWidth;

                QList< sk_sp<const SkImage> > backgroundLayers;
                if (!textSymbol->shieldResourceName.isEmpty())
                {
                    sk_sp<const SkImage> shield;
                    env->obtainTextShield(textSymbol->shieldResourceName, shield);

                    if (shield)
                        backgroundLayers.push_back(shield);
                }
                if (!textSymbol->underlayIconResourceName.isEmpty())
                {
                    sk_sp<const SkImage> icon;
                    env->obtainMapIcon(textSymbol->underlayIconResourceName, icon);
                    if (icon)
                        backgroundLayers.push_back(icon);
                }

                style.backgroundImage = SkiaUtilities::mergeImages(backgroundLayers);
                if (!qFuzzyCompare(textSymbol->scaleFactor, 1.0f) && style.backgroundImage)
                {
                    style.backgroundImage = SkiaUtilities::scaleImage(
                        style.backgroundImage,
                        textSymbol->scaleFactor,
                        textSymbol->scaleFactor);
                }

                style
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
                const auto rasterizedText = owner->textRasterizer->rasterize(
                    textSymbol->value,
                    style,
                    textSymbol->drawOnPath ? &glyphsWidth : nullptr,
                    &symbolExtraTopSpace,
                    &symbolExtraBottomSpace,
                    &lineSpacing,
                    &fontAscent);
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
                    rasterizedSymbol->content = textSymbol->value;
                    rasterizedSymbol->languageId = textSymbol->languageId;
                    rasterizedSymbol->minDistance = textSymbol->minDistance;
                    rasterizedSymbol->glyphsWidth = glyphsWidth;
                    //group->symbols.push_back(qMove(rasterizedSymbol));
                    orderMapSymbols.insert(std::pair<int, std::shared_ptr<const RasterizedSymbol>>(rasterizedSymbol->order, rasterizedSymbol));
                    mapGroups.insert(std::pair<std::shared_ptr<const RasterizedSymbol>, const std::shared_ptr<RasterizedSymbolsGroup>>(rasterizedSymbol, group));
                    isSymbolsAdded = true;
                }
                else
                {
                    // Calculate local offset. Since offset specifies center, it's a sum of
                    //  - vertical offset
                    //  - extra top space (which should be in texture, but not rendered, since transparent)
                    //  - height / 2
                    // This calculation is used only if this symbol is not first. Otherwise only following is used:
                    //  - vertical offset
                    PointI localOffset;
                    localOffset.y += textSymbol->verticalOffset;
                    if (isSymbolsAdded && !textSymbol->drawAlongPath)
                    {
                        localOffset.y += symbolExtraTopSpace;
                        localOffset.y += rasterizedText->height() / 2;
                        if (textAfterIcon && symbolExtraTopSpace == 0)
                            localOffset.y += qCeil(-fontAscent / 2);
                        textAfterIcon = false;
                    }

                    // Increment total offset
                    if (!textSymbol->drawAlongPath)
                        totalOffset += localOffset;

                    // Publish new rasterized symbol
                    const std::shared_ptr<RasterizedSpriteSymbol> rasterizedSymbol(new RasterizedSpriteSymbol(group, textSymbol));
                    rasterizedSymbol->image = rasterizedText;
                    rasterizedSymbol->order = textSymbol->order;
                    rasterizedSymbol->contentType = RasterizedSymbol::ContentType::Text;
                    rasterizedSymbol->content = textSymbol->value;
                    rasterizedSymbol->languageId = textSymbol->languageId;
                    rasterizedSymbol->minDistance = textSymbol->minDistance;
                    rasterizedSymbol->location31 = textSymbol->location31;
                    rasterizedSymbol->offset = textSymbol->drawAlongPath ? localOffset : totalOffset;
                    rasterizedSymbol->drawAlongPath = textSymbol->drawAlongPath;
                    if (!qIsNaN(textSymbol->intersectionSizeFactor))
                    {
                        rasterizedSymbol->intersectionBBox = AreaI::fromCenterAndSize(PointI(), PointI(
                            static_cast<int>(textSymbol->intersectionSizeFactor * rasterizedText->width()),
                            static_cast<int>(textSymbol->intersectionSizeFactor * rasterizedText->height())));
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
                    //group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
                    orderMapSymbols.insert(std::pair<int, std::shared_ptr<const RasterizedSymbol>>(rasterizedSymbol->order, rasterizedSymbol));
                    mapGroups.insert(std::pair<std::shared_ptr<const RasterizedSymbol>, const std::shared_ptr<RasterizedSymbolsGroup>>(rasterizedSymbol, group));
                    isSymbolsAdded = true;

                    // Next symbol should also take into account:
                    //  - height / 2
                    //  - extra bottom space (which should be in texture, but not rendered, since transparent)
                    //  - spacing between lines
                    if (!textSymbol->drawAlongPath)
                    {
                        totalOffset.y += rasterizedText->height() / 2;
                        totalOffset.y += symbolExtraBottomSpace;
                        totalOffset.y += qCeil(lineSpacing);
                    }
                }
            }
            else if (const auto& iconSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::IconSymbol>(symbol))
            {
                sk_sp<const SkImage> icon;
                if (!env->obtainMapIcon(iconSymbol->resourceName, icon) || !icon)
                    continue;
                if (!qFuzzyCompare(iconSymbol->scaleFactor, 1.0f))
                {
                    icon = SkiaUtilities::scaleImage(
                        icon,
                        iconSymbol->scaleFactor,
                        iconSymbol->scaleFactor);
                }

                sk_sp<const SkImage> background;
                if (!iconSymbol->shieldResourceName.isEmpty())
                {
                    env->obtainIconShield(iconSymbol->shieldResourceName, background);

                    if (!qFuzzyCompare(iconSymbol->scaleFactor, 1.0f) && background)
                    {
                        background = SkiaUtilities::scaleImage(
                            background,
                            iconSymbol->scaleFactor,
                            iconSymbol->scaleFactor);
                    }
                }

                QList< sk_sp<const SkImage> > layers;
                if (background)
                    layers.push_back(background);
                for (const auto& overlayResourceName : constOf(iconSymbol->underlayResourceNames))
                {
                    sk_sp<const SkImage> underlay;
                    if (!env->obtainMapIcon(overlayResourceName, underlay) || !underlay)
                        continue;

                    layers.push_back(underlay);
                }
                layers.push_back(icon);
                for (const auto& overlayResourceName : constOf(iconSymbol->overlayResourceNames))
                {
                    sk_sp<const SkImage> overlay;
                    if (!env->obtainMapIcon(overlayResourceName, overlay) || !overlay)
                        continue;

                    layers.push_back(overlay);
                }

                // Compose final image
                const auto rasterizedIcon = SkiaUtilities::mergeImages(layers);

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

                // Calculate local offset. Since offset specifies center, it's a sum of
                //  - height / 2
                // This calculation is used only if this symbol is not first. Otherwise nothing is used.
                PointI localOffset;
                if (!qFuzzyIsNull(iconSymbol->offsetFactor.x))
                    localOffset.x = qRound(iconSymbol->offsetFactor.x * rasterizedIcon->width());
                if (!qFuzzyIsNull(iconSymbol->offsetFactor.y))
                    localOffset.y = qRound(iconSymbol->offsetFactor.y * rasterizedIcon->height());
                if (isSymbolsAdded && !iconSymbol->drawAlongPath)
                    localOffset.y += rasterizedIcon->height() / 2;

                // Increment total offset
                if (!iconSymbol->drawAlongPath)
                    totalOffset += localOffset;

                // Publish new rasterized symbol
                const std::shared_ptr<RasterizedSpriteSymbol> rasterizedSymbol(new RasterizedSpriteSymbol(group, iconSymbol));
                rasterizedSymbol->image = rasterizedIcon;
                rasterizedSymbol->order = iconSymbol->order;
                rasterizedSymbol->contentType = RasterizedSymbol::ContentType::Icon;
                rasterizedSymbol->content = iconSymbol->resourceName;
                rasterizedSymbol->languageId = LanguageId::Invariant;
                rasterizedSymbol->minDistance = iconSymbol->minDistance;
                rasterizedSymbol->location31 = iconSymbol->location31;
                rasterizedSymbol->offset = iconSymbol->drawAlongPath ? localOffset : totalOffset;
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
                        static_cast<int>(iconSymbol->intersectionSize),
                        static_cast<int>(iconSymbol->intersectionSize)));
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
                //group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
                orderMapSymbols.insert(std::pair<int, std::shared_ptr<const RasterizedSymbol>>(rasterizedSymbol->order, rasterizedSymbol));
                mapGroups.insert(std::pair<std::shared_ptr<const RasterizedSymbol>, const std::shared_ptr<RasterizedSymbolsGroup>>(rasterizedSymbol, group));
                isSymbolsAdded = true;

                // Next symbol should also take into account:
                //  - height / 2
                if (!iconSymbol->drawAlongPath)
                {
                    totalOffset.y += rasterizedIcon->height() / 2;
                    textAfterIcon = true;
                }
            }
        }

        // Add group to output
        //outSymbolsGroups.push_back(qMove(group));
        symbolsContainer.push_back(orderMapSymbols);
    }
    
    QuadTree< std::shared_ptr<const RasterizedSymbol>, AreaI::CoordType> intersections;
    for (auto i = symbolsContainer.begin(); i != symbolsContainer.end(); ++i)
    {
        auto map = *i;
        for (const auto &m : map)
        {
            const auto & symbol = m.second;
            const auto & group = mapGroups.at(symbol);
            if (const auto& s = std::dynamic_pointer_cast<const RasterizedSpriteSymbol>(symbol))
            {
                QList<std::shared_ptr<const RasterizedSymbol>> outList;
                intersections.query(s->intersectionBBox, outList);
                if (outList.isEmpty()) {
                    group->symbols.push_back(symbol);
                }
                if (s->intersectionBBox.width() > 0 && s->intersectionBBox.height() > 0)
                {
                    intersections.insert(symbol, s->intersectionBBox);
                }
            }
            else
            {
                group->symbols.push_back(symbol);
            }
        }
    }
    
    for (const auto & g : mapGroups)
    {
        outSymbolsGroups.push_back(g.second);
    }
}
