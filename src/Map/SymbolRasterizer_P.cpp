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
        const auto constructedGroup = new RasterizedSymbolsGroup(symbolsEntry.key());
        std::shared_ptr<const RasterizedSymbolsGroup> group(constructedGroup);

        // Total offset allows several symbols to stack into column.
        // Offset specifies center of symbol bitmap
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
                const auto rasterizedText = TextRasterizer::getInstance().rasterize(
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
                    // Publish new rasterized symbol
                    const auto rasterizedSymbol = new RasterizedOnPathSymbol(
                        group,
                        constructedGroup->mapObject,
                        qMove(rasterizedText),
                        textSymbol->order,
                        textSymbol->value,
                        textSymbol->languageId,
                        textSymbol->minDistance,
                        glyphsWidth);
                    assert(static_cast<bool>(rasterizedSymbol->bitmap));
                    constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));
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
                    if (!constructedGroup->symbols.isEmpty())
                    {
                        localOffset.y += symbolExtraTopSpace;
                        localOffset.y += rasterizedText->height() / 2;
                    }

                    // Increment total offset
                    totalOffset += localOffset;

                    // Publish new rasterized symbol
                    const auto rasterizedSymbol = new RasterizedSpriteSymbol(
                        group,
                        constructedGroup->mapObject,
                        qMove(rasterizedText),
                        textSymbol->order,
                        textSymbol->value,
                        textSymbol->languageId,
                        textSymbol->minDistance,
                        textSymbol->location31,
                        totalOffset);
                    assert(static_cast<bool>(rasterizedSymbol->bitmap));
                    constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

                    // Next symbol should also take into account:
                    //  - height / 2
                    //  - extra bottom space (which should be in texture, but not rendered, since transparent)
                    //  - spacing between lines
                    totalOffset.y += rasterizedText->height() / 2;
                    totalOffset.y += symbolExtraBottomSpace;
                    totalOffset.y += qCeil(lineSpacing);
                }
            }
            else if (const auto& iconSymbol = std::dynamic_pointer_cast<const Primitiviser::IconSymbol>(symbol))
            {
                std::shared_ptr<const SkBitmap> bitmap;
                if (!env->obtainMapIcon(iconSymbol->resourceName, bitmap) || !bitmap)
                    continue;

#if OSMAND_DUMP_SYMBOLS
                {
                    QDir::current().mkpath("icon_symbols");
                    std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                    QString filename;
                    filename.sprintf("%s\\icon_symbols\\%p.png", qPrintable(QDir::currentPath()), bitmap.get());
                    encoder->encodeFile(qPrintable(filename), *bitmap, 100);
                }
#endif // OSMAND_DUMP_SYMBOLS

                // Calculate local offset. Since offset specifies center, it's a sum of
                //  - height / 2
                // This calculation is used only if this symbol is not first. Otherwise nothing is used.
                PointI localOffset;
                if (!constructedGroup->symbols.isEmpty())
                    localOffset.y += bitmap->height() / 2;

                // Increment total offset
                totalOffset += localOffset;

                // Publish new rasterized symbol
                const auto rasterizedSymbol = new RasterizedSpriteSymbol(
                    group,
                    constructedGroup->mapObject,
                    qMove(bitmap),
                    iconSymbol->order,
                    iconSymbol->resourceName,
                    LanguageId::Invariant,
                    PointI(),
                    iconSymbol->location31,
                    totalOffset);
                assert(static_cast<bool>(rasterizedSymbol->bitmap));
                constructedGroup->symbols.push_back(qMove(std::shared_ptr<const RasterizedSymbol>(rasterizedSymbol)));

                // Next symbol should also take into account:
                //  - height / 2
                totalOffset.y += bitmap->height() / 2;
            }
        }

        // Add group to output
        outSymbolsGroups.push_back(qMove(group));
    }
}
