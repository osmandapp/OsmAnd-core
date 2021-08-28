#include "SymbolRasterizer_P.h"
#include "SymbolRasterizer.h"

#include "QtCommon.h"
#include <QReadWriteLock>

#include "ignore_warnings_on_external_includes.h"
#include <SkColorFilter.h>
#include <SkBitmapDevice.h>
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

//#define OSMAND_DUMP_SYMBOLS 1
#if !defined(OSMAND_DUMP_SYMBOLS)
#   define OSMAND_DUMP_SYMBOLS 0
#endif // !defined(OSMAND_DUMP_SYMBOLS)

#if OSMAND_DUMP_SYMBOLS
#   include <SkImageEncoder.h>
#endif // OSMAND_DUMP_SYMBOLS

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

                QList< std::shared_ptr<const SkBitmap> > backgroundLayers;
                if (!textSymbol->shieldResourceName.isEmpty())
                {
                    std::shared_ptr<const SkBitmap> shield;
                    env->obtainTextShield(textSymbol->shieldResourceName, shield);

                    if (shield)
                        backgroundLayers.push_back(shield);
                }
                if (!textSymbol->underlayIconResourceName.isEmpty())
                {
                    std::shared_ptr<const SkBitmap> icon;
                    env->obtainMapIcon(textSymbol->underlayIconResourceName, icon);
                    if (icon)
                        backgroundLayers.push_back(icon);
                }

                style.backgroundBitmap = SkiaUtilities::mergeBitmaps(backgroundLayers);
                if (!qFuzzyCompare(textSymbol->scaleFactor, 1.0f) && style.backgroundBitmap)
                {
                    style.backgroundBitmap = SkiaUtilities::scaleBitmap(
                        style.backgroundBitmap,
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
                    std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                    QString filename;
                    filename.sprintf("%s\\text_symbols\\%p.png", qPrintable(QDir::currentPath()), rasterizedText.get());
                    encoder->encodeFile(qPrintable(filename), *rasterizedText.get(), 100);
                }
#endif // OSMAND_DUMP_SYMBOLS

                if (textSymbol->drawOnPath)
                {
                    // Publish new rasterized symbol
                    const std::shared_ptr<RasterizedOnPathSymbol> rasterizedSymbol(new RasterizedOnPathSymbol(
                        group,
                        textSymbol));
                    rasterizedSymbol->bitmap = qMove(rasterizedText);
                    rasterizedSymbol->order = textSymbol->order;
                    rasterizedSymbol->contentType = RasterizedSymbol::ContentType::Text;
                    rasterizedSymbol->content = textSymbol->value;
                    rasterizedSymbol->languageId = textSymbol->languageId;
                    rasterizedSymbol->minDistance = textSymbol->minDistance;
                    rasterizedSymbol->glyphsWidth = glyphsWidth;
                    group->symbols.push_back(qMove(rasterizedSymbol));
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
                    if (!group->symbols.isEmpty() && !textSymbol->drawAlongPath)
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
                    rasterizedSymbol->bitmap = rasterizedText;
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
                    group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

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
                std::shared_ptr<const SkBitmap> iconBitmap;
                if (!env->obtainMapIcon(iconSymbol->resourceName, iconBitmap) || !iconBitmap)
                    continue;
                if (!qFuzzyCompare(iconSymbol->scaleFactor, 1.0f))
                {
                    iconBitmap = SkiaUtilities::scaleBitmap(
                        iconBitmap,
                        iconSymbol->scaleFactor,
                        iconSymbol->scaleFactor);
                }

                std::shared_ptr<const SkBitmap> backgroundBitmap;
                if (!iconSymbol->shieldResourceName.isEmpty())
                {
                    env->obtainIconShield(iconSymbol->shieldResourceName, backgroundBitmap);

                    if (!qFuzzyCompare(iconSymbol->scaleFactor, 1.0f) && backgroundBitmap)
                    {
                        backgroundBitmap = SkiaUtilities::scaleBitmap(
                            backgroundBitmap,
                            iconSymbol->scaleFactor,
                            iconSymbol->scaleFactor);
                    }
                }

                QList< std::shared_ptr<const SkBitmap> > layers;
                if (backgroundBitmap)
                    layers.push_back(backgroundBitmap);
                for (const auto& overlayResourceName : constOf(iconSymbol->underlayResourceNames))
                {
                    std::shared_ptr<const SkBitmap> underlayBitmap;
                    if (!env->obtainMapIcon(overlayResourceName, underlayBitmap) || !underlayBitmap)
                        continue;

                    layers.push_back(underlayBitmap);
                }
                layers.push_back(iconBitmap);
                for (const auto& overlayResourceName : constOf(iconSymbol->overlayResourceNames))
                {
                    std::shared_ptr<const SkBitmap> overlayBitmap;
                    if (!env->obtainMapIcon(overlayResourceName, overlayBitmap) || !overlayBitmap)
                        continue;

                    layers.push_back(overlayBitmap);
                }

                // Compose final image
                const auto rasterizedIcon = SkiaUtilities::mergeBitmaps(layers);

#if OSMAND_DUMP_SYMBOLS
                {
                    QDir::current().mkpath("icon_symbols");
                    std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                    QString filename;
                    filename.sprintf("%s\\icon_symbols\\%p.png", qPrintable(QDir::currentPath()), rasterizedIcon.get());
                    encoder->encodeFile(qPrintable(filename), *rasterizedIcon, 100);
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
                if (!group->symbols.isEmpty() && !iconSymbol->drawAlongPath)
                    localOffset.y += rasterizedIcon->height() / 2;

                // Increment total offset
                if (!iconSymbol->drawAlongPath)
                    totalOffset += localOffset;

                // Publish new rasterized symbol
                const std::shared_ptr<RasterizedSpriteSymbol> rasterizedSymbol(new RasterizedSpriteSymbol(group, iconSymbol));
                rasterizedSymbol->bitmap = rasterizedIcon;
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
                group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

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
        outSymbolsGroups.push_back(qMove(group));
    }
}
