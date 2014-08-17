#include "SymbolRasterizer_P.h"
#include "SymbolRasterizer.h"

#include "QtCommon.h"
#include <QReadWriteLock>

#include "ignore_warnings_on_external_includes.h"
#include <SkBlurDrawLooper.h>
#include <SkColorFilter.h>
#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkError.h>
#include <SkBitmapDevice.h>
#include "restore_internal_warnings.h"

#include "MapPresentationEnvironment.h"
#include "MapStyleEvaluationResult.h"
#include "Primitiviser.h"
#include "TextRasterizer.h"
#include "BinaryMapObject.h"
#include "ObfMapSectionInfo.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Stopwatch.h"
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
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter,
    const IQueryController* const controller)
{
    const auto& env = primitivizedArea->mapPresentationEnvironment;

    for (const auto& symbolsEntry : rangeOf(constOf(primitivizedArea->symbolsBySourceObjects)))
    {
        if (controller && controller->isAborted())
            return;

        // Apply filter, if it's present
        if (filter && !filter(symbolsEntry.key()))
            continue;

        // Create group
        const std::shared_ptr<RasterizedSymbolsGroup> group(new RasterizedSymbolsGroup(symbolsEntry.key()));

        // Total offset allows several symbols to stack into column. Offset specifies center of symbol bitmap.
        // This offset is computed only in case symbol is not on-path and not along-path
        PointI totalOffset;

        for (const auto& symbol : constOf(symbolsEntry.value()))
        {
            if (controller && controller->isAborted())
                return;

            if (const auto& textSymbol = std::dynamic_pointer_cast<const Primitiviser::TextSymbol>(symbol))
            {
                TextRasterizer::Style style;
                if (!textSymbol->drawOnPath && textSymbol->shieldResourceName.isEmpty())
                    style.wrapWidth = textSymbol->wrapWidth;
                if (!textSymbol->shieldResourceName.isEmpty())
                    env->obtainTextShield(textSymbol->shieldResourceName, style.backgroundBitmap);
                style
                    .setBold(textSymbol->isBold)
                    .setColor(textSymbol->color)
                    .setSize(textSymbol->size);

                if (textSymbol->shadowRadius > 0)
                {
                    style
                        .setHaloColor(textSymbol->shadowColor)
                        .setHaloRadius(textSymbol->shadowRadius + 2 /*px*/);
                    //NOTE: ^^^ This is same as specifying 'x:2' in style, but due to backward compatibility with Android, leave as-is
                }

                float lineSpacing;
                float symbolExtraTopSpace;
                float symbolExtraBottomSpace;
                QVector<SkScalar> glyphsWidth;
                const auto rasterizedText = TextRasterizer::globalInstance().rasterize(
                    textSymbol->value,
                    style,
                    textSymbol->drawOnPath ? &glyphsWidth : nullptr,
                    &symbolExtraTopSpace,
                    &symbolExtraBottomSpace,
                    &lineSpacing);

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
                    if (textSymbol->drawOnPath && textSymbol->verticalOffset > 0)
                    {
                        LogPrintf(LogSeverityLevel::Warning, "Hey, on-path + vertical offset is not supported!");
                        assert(false);
                    }

                    // Publish new rasterized symbol
                    const std::shared_ptr<RasterizedOnPathSymbol> rasterizedSymbol(new RasterizedOnPathSymbol(group, textSymbol));
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
                    rasterizedSymbol->intersectionSize = PointI(-1, -1);
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
            else if (const auto& iconSymbol = std::dynamic_pointer_cast<const Primitiviser::IconSymbol>(symbol))
            {
                std::shared_ptr<const SkBitmap> iconBitmap;
                if (!env->obtainMapIcon(iconSymbol->resourceName, iconBitmap) || !iconBitmap)
                    continue;
                std::shared_ptr<const SkBitmap> backgroundBitmap;
                if (!iconSymbol->shieldResourceName.isEmpty())
                    env->obtainIconShield(iconSymbol->shieldResourceName, backgroundBitmap);

                // Compose final image
                std::shared_ptr<const SkBitmap> rasterizedIcon;
                if (!backgroundBitmap)
                    rasterizedIcon = iconBitmap;
                else
                {
                    // Compute composed image size
                    const auto rasterizedIconWidth = qMax(iconBitmap->width(), backgroundBitmap->width());
                    const auto rasterizedIconHeight = qMax(iconBitmap->height(), backgroundBitmap->height());

                    // Create a bitmap that will be hold entire symbol
                    const auto pRasterizedIcon = new SkBitmap();
                    rasterizedIcon.reset(pRasterizedIcon);
                    pRasterizedIcon->setConfig(SkBitmap::kARGB_8888_Config, rasterizedIconWidth, rasterizedIconHeight);
                    pRasterizedIcon->allocPixels();
                    pRasterizedIcon->eraseColor(SK_ColorTRANSPARENT);
                    
                    // Create canvas
                    SkBitmapDevice target(*pRasterizedIcon);
                    SkCanvas canvas(&target);

                    // Draw the background
                    canvas.drawBitmap(*backgroundBitmap,
                        (rasterizedIconWidth - backgroundBitmap->width()) / 2.0f,
                        (rasterizedIconHeight - backgroundBitmap->height()) / 2.0f,
                        nullptr);

                    // Draw the icon
                    canvas.drawBitmap(*iconBitmap,
                        (rasterizedIconWidth - iconBitmap->width()) / 2.0f,
                        (rasterizedIconHeight - iconBitmap->height()) / 2.0f,
                        nullptr);

                    // Flush all operations
                    canvas.flush();
                }

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
                rasterizedSymbol->minDistance = PointI();
                rasterizedSymbol->location31 = iconSymbol->location31;
                rasterizedSymbol->offset = iconSymbol->drawAlongPath ? localOffset : totalOffset;
                rasterizedSymbol->drawAlongPath = iconSymbol->drawAlongPath;
                rasterizedSymbol->intersectionSize = PointI(iconSymbol->intersectionSize, iconSymbol->intersectionSize);
                group->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

                // Next symbol should also take into account:
                //  - height / 2
                if (!iconSymbol->drawAlongPath)
                    totalOffset.y += rasterizedIcon->height() / 2;
            }
        }

        // Add group to output
        outSymbolsGroups.push_back(qMove(group));
    }
}
