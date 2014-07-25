#include "MapPresentationEnvironment_P.h"
#include "MapPresentationEnvironment.h"

#include <limits>

#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkImageDecoder.h>
#include <SkStream.h>
#include <SkTypeface.h>

#include "MapStyle_P.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "ObfMapSectionInfo.h"
#include "EmbeddedResources.h"
#include "IExternalResourcesProvider.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapPresentationEnvironment_P::MapPresentationEnvironment_P(MapPresentationEnvironment* owner_)
    : owner(owner_)
    , dummyMapSection(new ObfMapSectionInfo())
{
}

OsmAnd::MapPresentationEnvironment_P::~MapPresentationEnvironment_P()
{
    {
        QMutexLocker scopedLocker(&_shadersBitmapsMutex);

        _shadersBitmaps.clear();
    }

    {
        QMutexLocker scopedLocker(&_pathEffectsMutex);

        for (auto& pathEffect : _pathEffects)
            pathEffect->unref();
    }
}

void OsmAnd::MapPresentationEnvironment_P::initializeOneWayPaint(SkPaint& paint)
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xff6c70d5);
}

void OsmAnd::MapPresentationEnvironment_P::initialize()
{
    _mapPaint.setAntiAlias(true);

    _textPaint.setAntiAlias(true);
    _textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    static_assert(sizeof(QChar) == 2, "If QChar is not 2 bytes, then encoding is not kUTF16_TextEncoding");

    // Fonts register (in priority order):
    _fontsRegister.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Italic.ttf"), false, true });
    _fontsRegister.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Semibold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-SemiboldItalic.ttf"), true, true });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansEthiopic-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansEthiopic-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansArabic.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansHebrew-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansHebrew-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansThai-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansThai-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansArmenian.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansGeorgian.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansDevanagari-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansDevanagari-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansTamil-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansTamil-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansMalayalam.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansMalayalam-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansBengali-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansBengali-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansTelugu-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansTelugu-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansKannada-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansKannada-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansKhmer-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansKhmer-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansLao-Regular.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/NotoSansLao-Bold.ttf"), true, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/DroidSansFallback.ttf"), false, false });
    _fontsRegister.push_back({ QLatin1String("map/fonts/MTLmr3m.ttf"), false, false });

    _roadDensityZoomTile = 0;
    _roadsDensityLimitPerTile = 0;
    _shadowRenderingMode = 0;
    _shadowRenderingColor = ColorRGB(0x96, 0x96, 0x96);
    _polygonMinSizeToDisplay = 0.0;
    _defaultBgColor = ColorRGB(0xf1, 0xee, 0xe8);

    owner->style->resolveAttribute(QLatin1String("defaultColor"), _attributeRule_defaultColor);
    owner->style->resolveAttribute(QLatin1String("shadowRendering"), _attributeRule_shadowRendering);
    owner->style->resolveAttribute(QLatin1String("polygonMinSizeToDisplay"), _attributeRule_polygonMinSizeToDisplay);
    owner->style->resolveAttribute(QLatin1String("roadDensityZoomTile"), _attributeRule_roadDensityZoomTile);
    owner->style->resolveAttribute(QLatin1String("roadsDensityLimitPerTile"), _attributeRule_roadsDensityLimitPerTile);

    const auto intervalW = 0.5f*owner->displayDensityFactor + 0.5f; // 0.5dp + 0.5px

    {
        const float intervals_oneway[4][4] =
        {
            { 0, 12, 10 * intervalW, 152 },
            { 0, 12, 9 * intervalW, 152 + intervalW },
            { 0, 12 + 6 * intervalW, 2 * intervalW, 152 + 2 * intervalW },
            { 0, 12 + 6 * intervalW, 1 * intervalW, 152 + 3 * intervalW }
        };
        SkPathEffect* arrowDashEffect1 = new SkDashPathEffect(intervals_oneway[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = new SkDashPathEffect(intervals_oneway[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = new SkDashPathEffect(intervals_oneway[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = new SkDashPathEffect(intervals_oneway[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f * intervalW);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f * intervalW);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f * intervalW);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f * intervalW);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _oneWayPaints.push_back(qMove(paint));
        }
    }

    {
        const float intervals_reverse[4][4] =
        {
            { 0, 12, 10 * intervalW, 152 },
            { 0, 12 + 1 * intervalW, 9 * intervalW, 152 },
            { 0, 12 + 2 * intervalW, 2 * intervalW, 152 + 6 * intervalW },
            { 0, 12 + 3 * intervalW, 1 * intervalW, 152 + 6 * intervalW }
        };
        SkPathEffect* arrowDashEffect1 = new SkDashPathEffect(intervals_reverse[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = new SkDashPathEffect(intervals_reverse[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = new SkDashPathEffect(intervals_reverse[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = new SkDashPathEffect(intervals_reverse[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f * intervalW);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f * intervalW);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f * intervalW);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f * intervalW);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _reverseOneWayPaints.push_back(qMove(paint));
        }
    }
}

void OsmAnd::MapPresentationEnvironment_P::clearFontsCache()
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    for (const auto& typeface : _fontTypefacesCache)
        typeface->unref();
    _fontTypefacesCache.clear();
}

SkTypeface* OsmAnd::MapPresentationEnvironment_P::getTypefaceForFontResource(const QString& fontResource) const
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    // Look for loaded typefaces
    const auto& citTypeface = _fontTypefacesCache.constFind(fontResource);
    if (citTypeface != _fontTypefacesCache.cend())
        return *citTypeface;

    // Load raw data from resource
    const auto fontData = EmbeddedResources::decompressResource(fontResource);
    if (fontData.isNull())
        return nullptr;

    // Load typeface from font data
    const auto fontDataStream = new SkMemoryStream(fontData.constData(), fontData.length(), true);
    const auto typeface = SkTypeface::CreateFromStream(fontDataStream);
    fontDataStream->unref();
    if (!typeface)
        return nullptr;

    _fontTypefacesCache.insert(fontResource, typeface);

    return typeface;
}

void OsmAnd::MapPresentationEnvironment_P::configurePaintForText(SkPaint& paint, const QString& text, const bool bold, const bool italic) const
{
    // Lookup matching font entry
    const FontsRegisterEntry* pBestMatchEntry = nullptr;
    SkTypeface* bestMatchTypeface = nullptr;
    SkPaint textCoverageTestPaint;
    textCoverageTestPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    for (const auto& entry : constOf(_fontsRegister))
    {
        // Get typeface for this entry
        const auto typeface = getTypefaceForFontResource(entry.resource);
        if (!typeface)
            continue;

        // Check if this typeface covers provided text
        textCoverageTestPaint.setTypeface(typeface);
        if (!textCoverageTestPaint.containsText(text.constData(), text.length()*sizeof(QChar)))
            continue;

        // Mark this as best match
        pBestMatchEntry = &entry;
        bestMatchTypeface = typeface;

        // If this entry fully matches the request, stop search
        if (entry.bold == bold && entry.italic == italic)
            break;
    }

    // If there's no best match, fallback to default typeface
    if (bestMatchTypeface == nullptr)
    {
        paint.setTypeface(nullptr);

        // Adjust to handle bold text
        if (bold)
            paint.setFakeBoldText(true);

        return;
    }

    // There was a best match, use that
    paint.setTypeface(bestMatchTypeface);

    // Adjust to handle bold text
    if (pBestMatchEntry->bold != bold && bold)
        paint.setFakeBoldText(true);
}

QHash< std::shared_ptr<const OsmAnd::MapStyleValueDefinition>, OsmAnd::MapStyleValue > OsmAnd::MapPresentationEnvironment_P::getSettings() const
{
    return _settings;
}

void OsmAnd::MapPresentationEnvironment_P::setSettings(const QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings)
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    _settings = newSettings;
}

void OsmAnd::MapPresentationEnvironment_P::setSettings(const QHash< QString, QString >& newSettings)
{
    QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > resolvedSettings;
    resolvedSettings.reserve(newSettings.size());

    for (const auto& itSetting : rangeOf(newSettings))
    {
        const auto& name = itSetting.key();
        const auto& value = itSetting.value();

        // Resolve input-value definition by name
        std::shared_ptr<const MapStyleValueDefinition> inputValueDef;
        if (!owner->style->resolveValueDefinition(name, inputValueDef))
        {
            LogPrintf(LogSeverityLevel::Warning, "Setting of '%s' to '%s' impossible: failed to resolve input value definition failed with such name");
            continue;
        }

        // Parse value
        MapStyleValue parsedValue;
        if (!owner->style->_p->parseValue(inputValueDef, value, parsedValue))
        {
            LogPrintf(LogSeverityLevel::Warning, "Setting of '%s' to '%s' impossible: failed to parse value");
            continue;
        }

        resolvedSettings.insert(inputValueDef, parsedValue);
    }

    setSettings(resolvedSettings);
}

void OsmAnd::MapPresentationEnvironment_P::applyTo(MapStyleEvaluator& evaluator) const
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    for (const auto& settingEntry : rangeOf(constOf(_settings)))
    {
        const auto& valueDef = settingEntry.key();
        const auto& settingValue = settingEntry.value();

        switch (valueDef->dataType)
        {
        case MapStyleValueDataType::Integer:
            evaluator.setIntegerValue(valueDef->id,
                settingValue.isComplex
                ? settingValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                : settingValue.asSimple.asInt);
            break;
        case MapStyleValueDataType::Float:
            evaluator.setFloatValue(valueDef->id,
                settingValue.isComplex
                ? settingValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                : settingValue.asSimple.asFloat);
            break;
        case MapStyleValueDataType::Boolean:
        case MapStyleValueDataType::String:
        case MapStyleValueDataType::Color:
            assert(!settingValue.isComplex);
            evaluator.setIntegerValue(valueDef->id, settingValue.asSimple.asUInt);
            break;
        }
    }
}

bool OsmAnd::MapPresentationEnvironment_P::obtainBitmapShader(const QString& name, SkBitmapProcShader* &outShader) const
{
    QMutexLocker scopedLocker(&_shadersBitmapsMutex);

    auto itShaderBitmap = _shadersBitmaps.constFind(name);
    if (itShaderBitmap == _shadersBitmaps.cend())
    {
        const auto shaderBitmapPath = QString::fromLatin1("map/shaders/%1.png").arg(name);

        // Get data from embedded resources
        const auto data = obtainResourceByName(shaderBitmapPath);

        // Decode bitmap for a shader
        const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;
        itShaderBitmap = _shadersBitmaps.insert(name, bitmap);
    }

    // Create shader from that bitmap
    outShader = new SkBitmapProcShader(*itShaderBitmap->get(), SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    return true;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainPathEffect(const QString& encodedPathEffect, SkPathEffect* &outPathEffect) const
{
    QMutexLocker scopedLocker(&_pathEffectsMutex);

    auto itPathEffects = _pathEffects.constFind(encodedPathEffect);
    if (itPathEffects == _pathEffects.cend())
    {
        const auto& strIntervals = encodedPathEffect.split('_', QString::SkipEmptyParts);
        const auto intervalsCount = strIntervals.size();

        const auto intervals = new SkScalar[intervalsCount];
        auto pInterval = intervals;
        for (const auto& strInterval : constOf(strIntervals))
        {
            float computedValue = 0.0f;

            if (!strInterval.contains(':'))
            {
                computedValue = strInterval.toFloat()*owner->displayDensityFactor;
            }
            else
            {
                // 'dip:px' format
                const auto& complexValue = strInterval.split(':', QString::KeepEmptyParts);

                computedValue = complexValue[0].toFloat()*owner->displayDensityFactor + complexValue[1].toFloat();
            }

            *(pInterval++) = computedValue;
        }

        // Validate
        if (intervalsCount < 2 || intervalsCount % 2 != 0)
        {
            LogPrintf(LogSeverityLevel::Warning, "Path effect (%s) with %d intervals is invalid", qPrintable(encodedPathEffect), intervalsCount);
            return false;
        }

        SkPathEffect* pathEffect = new SkDashPathEffect(intervals, intervalsCount, 0);
        delete[] intervals;

        itPathEffects = _pathEffects.insert(encodedPathEffect, pathEffect);
    }

    outPathEffect = *itPathEffects;
    return true;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainMapIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const
{
    QMutexLocker scopedLocker(&_mapIconsMutex);

    auto itIcon = _mapIcons.constFind(name);
    if (itIcon == _mapIcons.cend())
    {
        const auto bitmapPath = QString::fromLatin1("map/map_icons/%1.png").arg(name);

        // Get data from embedded resources
        auto data = obtainResourceByName(bitmapPath);

        // Decode data
        const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        itIcon = _mapIcons.insert(name, bitmap);
    }

    outIcon = *itIcon;
    return true;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainTextShield(const QString& name, std::shared_ptr<const SkBitmap>& outTextShield) const
{
    QMutexLocker scopedLocker(&_textShieldsMutex);

    auto itTextShield = _textShields.constFind(name);
    if (itTextShield == _textShields.cend())
    {
        const auto bitmapPath = QString::fromLatin1("map/shields/%1.png").arg(name);

        // Get data from embedded resources
        auto data = obtainResourceByName(bitmapPath);

        // Decode data
        const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        itTextShield = _textShields.insert(name, bitmap);
    }

    outTextShield = *itTextShield;
    return true;
}

QByteArray OsmAnd::MapPresentationEnvironment_P::obtainResourceByName(const QString& name) const
{
    // Try to obtain from external resources first
    if (static_cast<bool>(owner->externalResourcesProvider))
    {
        bool ok = false;
        const auto resource = owner->externalResourcesProvider->getResource(name, &ok);
        if (ok)
            return resource;
    }

    // Otherwise obtain from embedded
    return EmbeddedResources::decompressResource(name);
}

OsmAnd::ColorARGB OsmAnd::MapPresentationEnvironment_P::getDefaultBackgroundColor(const ZoomLevel zoom) const
{
    auto result = _defaultBgColor;

    if (_attributeRule_defaultColor)
    {
        MapStyleEvaluator evaluator(owner->style, owner->displayDensityFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult;
        if (evaluator.evaluate(_attributeRule_defaultColor, &evalResult))
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, result.argb);
    }

    return result;
}

void OsmAnd::MapPresentationEnvironment_P::obtainShadowRenderingOptions(const ZoomLevel zoom, int& mode, ColorARGB& color) const
{
    mode = _shadowRenderingMode;
    color = _shadowRenderingColor;

    if (_attributeRule_shadowRendering)
    {
        MapStyleEvaluator evaluator(owner->style, owner->displayDensityFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult;
        if (evaluator.evaluate(_attributeRule_shadowRendering, &evalResult))
        {
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, mode);
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, color.argb);
        }
    }
}

double OsmAnd::MapPresentationEnvironment_P::getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const
{
    auto result = _polygonMinSizeToDisplay;

    if (_attributeRule_polygonMinSizeToDisplay)
    {
        MapStyleEvaluator evaluator(owner->style, owner->displayDensityFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult;
        if (evaluator.evaluate(_attributeRule_polygonMinSizeToDisplay, &evalResult))
        {
            int polygonMinSizeToDisplay;
            if (evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, polygonMinSizeToDisplay))
                result = polygonMinSizeToDisplay;
        }
    }

    return result;
}

unsigned int OsmAnd::MapPresentationEnvironment_P::getRoadDensityZoomTile(const ZoomLevel zoom) const
{
    auto result = _roadDensityZoomTile;

    if (_attributeRule_roadDensityZoomTile)
    {
        MapStyleEvaluator evaluator(owner->style, owner->displayDensityFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult;
        if (evaluator.evaluate(_attributeRule_roadDensityZoomTile, &evalResult))
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, result);
    }

    return result;
}

unsigned int OsmAnd::MapPresentationEnvironment_P::getRoadsDensityLimitPerTile(const ZoomLevel zoom) const
{
    auto result = _roadsDensityLimitPerTile;

    if (_attributeRule_roadsDensityLimitPerTile)
    {
        MapStyleEvaluator evaluator(owner->style, owner->displayDensityFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult;
        if (evaluator.evaluate(_attributeRule_roadsDensityLimitPerTile, &evalResult))
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, result);
    }

    return result;
}
