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

    std::vector<const std::shared_ptr<const MapObject>> filteredMapObjects;
    auto combinedText = combineSimilarText(primitivisedObjects, filter, queryController, filteredMapObjects);
    
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
        if(std::find(filteredMapObjects.begin(), filteredMapObjects.end(), mapObject) != filteredMapObjects.end()){
            continue;
        }

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
                    
                    auto it = combinedText.find(textSymbol);
                    if (it != combinedText.end())
                    {
                        auto points31 = it.value();
                        rasterizedSymbol->combinedPoints31 = points31;
                        group->symbols.push_back(qMove(rasterizedSymbol));
                    }
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
                if (!group->symbols.isEmpty() && !iconSymbol->drawAlongPath)
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

QHash<std::shared_ptr<const OsmAnd::MapPrimitiviser::TextSymbol>, QVector<OsmAnd::PointI>> OsmAnd::SymbolRasterizer_P::combineSimilarText(
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
    const FilterByMapObject filter,
    const std::shared_ptr<const IQueryController>& queryController,
    std::vector<const std::shared_ptr<const MapObject>>& filteredMapObjects) const
{
    const auto& env = primitivisedObjects->mapPresentationEnvironment;
    PointD div31Pix = primitivisedObjects->scaleDivisor31ToPixel;
    float combineGap = env->displayDensityFactor * 45;
    float combineMaxLength = env->displayDensityFactor * 550;// max length
    QHash<std::shared_ptr<const OsmAnd::MapPrimitiviser::TextSymbol>, QVector<OsmAnd::PointI>> out;
    QHash<QString, QList<std::shared_ptr<CombineOnPathText>>> namesMap;

    
    for (const auto& symbolGroupEntry : rangeOf(constOf(primitivisedObjects->symbolsGroups)))
    {
        if (queryController && queryController->isAborted())
            return out;

        const auto& mapObject = symbolGroupEntry.key();
        const auto& symbolsGroup = symbolGroupEntry.value();

        if (filter && !filter(mapObject))
        {
            filteredMapObjects.push_back(mapObject);
            continue;
        }            

        for (const auto& symbol : constOf(symbolsGroup->symbols))
        {
            if (queryController && queryController->isAborted())
                return out;
            if (const auto& textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol))
            {
                if (!textSymbol->drawOnPath || mapObject->points31.size() < 2 || textSymbol->value.isEmpty()) {
                    continue;
                }
                std::shared_ptr<CombineOnPathText> combine = std::make_shared<CombineOnPathText>(textSymbol, mapObject->points31);
                QString key = textSymbol->value + QString::number(textSymbol->order);
                QHash< QString, QList<std::shared_ptr<CombineOnPathText>>>::iterator it = namesMap.find(key);
                if (it == namesMap.end())
                {
                    QList<std::shared_ptr<CombineOnPathText>> list;
                    list.append(combine);
                    namesMap.insert(key, list);
                }
                else
                {
                    auto& list = it.value();
                    list.append(combine);
                }
            }
        }
    }
    
    if (namesMap.size() == 0)
        return out;

    QVector<PointI> pointsS;
    QVector<PointI> pointsSCombine;
    QVector<PointI> pointsP;
    
    for (auto it = namesMap.begin(); it != namesMap.end(); it++)
    {
        QList<std::shared_ptr<CombineOnPathText>> list = it.value();
        int combined = 20;    // max combined

        bool combineOnIteration = true;
        if (list.size() > 1)
        {
            std::vector<float> distances;
            for (auto p = list.begin(); p != list.end(); p++)
            {
                distances.push_back(calcLength((*p)->points31, div31Pix));
            }

            while (combined > 0 && combineOnIteration)
            {
                combined--;
                combineOnIteration = false;
                int pi = 0;
                for (auto p = list.begin(); p != list.end() && !combineOnIteration; p++, pi++)
                {
                    if ((*p)->combined) continue;
                    pointsP = (*p)->points31;
                    if (distances[pi] > combineMaxLength) continue;

                    auto sToCombine = p;
                    float minGap = combineGap;
                    int siCombine = pi;
                    float gapMeasure = 0;

                    auto s = p;
                    int si = pi + 1;
                    for (s++; s != list.end(); s++, si++)
                    {
                        if ((*s)->combined || p == s) continue;
                        if (distances[si] > combineMaxLength) continue;
                        pointsS = (*s)->points31;
                        if (combine2Segments(pointsS, pointsP, (*s)->points31, combineGap, &gapMeasure, false, div31Pix) ||
                            combine2Segments(pointsP, pointsS, (*p)->points31, combineGap, &gapMeasure, false, div31Pix))
                        {
                            if (minGap > gapMeasure)
                            {
                                minGap = gapMeasure;
                                sToCombine = s;
                                siCombine = si;
                                pointsSCombine = pointsS;
                            }
                        }
                    }
                    if (sToCombine != p)
                    {
                        if (combine2Segments(pointsSCombine, pointsP, (*sToCombine)->points31, combineGap, &gapMeasure, true, div31Pix))
                        {
                            (*p)->combined = true;
                            combineOnIteration = true;
                            distances[siCombine] += distances[pi];

                        }
                        else if (combine2Segments(pointsP, pointsSCombine, (*p)->points31, combineGap, &gapMeasure, true, div31Pix))
                        {
                            (*sToCombine)->combined = true;
                            combineOnIteration = true;
                            distances[pi] += distances[siCombine];
                        }
                    }
                }
            }
        }
    }

    for (auto it = namesMap.begin(); it != namesMap.end(); it++)
    {
        QList<std::shared_ptr<CombineOnPathText>> list = it.value();
        for (auto p = list.begin(); p != list.end(); p++)
        {
            auto it = out.find((*p)->textSymbol);
            if (it == out.end())
            {
                if (!(*p)->combined)
                {
                    out.insert((*p)->textSymbol, (*p)->points31);
                }
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error, "Text symbol is not single: %s", qPrintable((*p)->textSymbol->value));
            }
            
        }
    }
    return out;
}

double OsmAnd::SymbolRasterizer_P::calcLength(QVector<PointI>& pointsP, PointD scaleDivisor31ToPixel) const
{
    double len = 0;
    double divX = scaleDivisor31ToPixel.x;
    double divY = scaleDivisor31ToPixel.y;
    for (int i = 1; i < pointsP.size(); i++)
    {
        double dx = (pointsP.at(i).x - pointsP.at(i - 1).x) / divX;
        double dy = (pointsP.at(i).y - pointsP.at(i - 1).y) / divY;
        len += sqrt(dx * dx + dy * dy);
    }
    return len;
}

bool OsmAnd::SymbolRasterizer_P::combine2Segments(QVector<PointI>& pointsS, QVector<PointI>& pointsP, QVector<PointI>& outPoints, float combineGap,
    float* gapMetric, bool combine, PointD scaleDivisor31ToPixel) const
{
    double divX = scaleDivisor31ToPixel.x;
    double divY = scaleDivisor31ToPixel.y;
    double px0 = pointsP.at(0).x / divX;
    double py0 = pointsP.at(0).y / divY;
    double sxl = pointsS.at(pointsS.size() - 1).x / scaleDivisor31ToPixel.x;
    double syl = pointsS.at(pointsS.size() - 1).y / scaleDivisor31ToPixel.y;
    *gapMetric = abs(px0 - sxl) + abs(py0 - syl);
    if (*gapMetric < combineGap)
    {
        // calculate scalar product to maker sure connecting good line
        double px1 = pointsP.at(1).x / divX;
        double py1 = pointsP.at(1).y / divY;
        double sxl1 = pointsS.at(pointsS.size() - 2).x / divX;
        double syl1 = pointsS.at(pointsS.size() - 2).y / divY;
        double pvx = (px1 - px0) / sqrt((px1 - px0) * (px1 - px0) + (py1 - py0) * (py1 - py0));
        double pvy = (py1 - py0) / sqrt((px1 - px0) * (px1 - px0) + (py1 - py0) * (py1 - py0));
        double svx = (sxl - sxl1) / sqrt((sxl - sxl1) * (sxl - sxl1) + (syl - syl1) * (syl - syl1));
        double svy = (syl - syl1) / sqrt((sxl - sxl1) * (sxl - sxl1) + (syl - syl1) * (syl - syl1));

        if (pvx * svx + svy * pvy <= 0)
        {
            return false;
        }
        if (combine)
        {
            for (int k = 1; k < pointsP.size(); k++)
            {
                outPoints.push_back(pointsP.at(k));
            }
        }
        return true;
    }
    return false;
}
