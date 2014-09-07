#include "MapPresentationEnvironment_P.h"
#include "MapPresentationEnvironment.h"

#include <limits>

#include "ignore_warnings_on_external_includes.h"
#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkImageDecoder.h>
#include <SkStream.h>
#include "restore_internal_warnings.h"

#include "MapStyle_P.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleValue.h"
#include "ObfMapSectionInfo.h"
#include "CoreResourcesEmbeddedBundle.h"
#include "ICoreResourcesProvider.h"
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
}

void OsmAnd::MapPresentationEnvironment_P::initialize()
{
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
            LogPrintf(LogSeverityLevel::Warning,
                "Setting of '%s' to '%s' impossible: failed to resolve input value definition failed with such name",
                qPrintable(name),
                qPrintable(value));
            continue;
        }

        // Parse value
        MapStyleValue parsedValue;
        if (!owner->style->_p->parseValue(inputValueDef, value, parsedValue))
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Setting of '%s' to '%s' impossible: failed to parse value",
                qPrintable(name),
                qPrintable(value));
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

bool OsmAnd::MapPresentationEnvironment_P::obtainShaderBitmap(const QString& name, std::shared_ptr<const SkBitmap>& outShaderBitmap) const
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
    outShaderBitmap = *itShaderBitmap;
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

bool OsmAnd::MapPresentationEnvironment_P::obtainIconShield(const QString& name, std::shared_ptr<const SkBitmap>& outIconShield) const
{
    QMutexLocker scopedLocker(&_iconShieldsMutex);

    auto itIconShield = _iconShields.constFind(name);
    if (itIconShield == _iconShields.cend())
    {
        const auto bitmapPath = QString::fromLatin1("map/shields/%1.png").arg(name);

        // Get data from embedded resources
        auto data = obtainResourceByName(bitmapPath);

        // Decode data
        const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        itIconShield = _iconShields.insert(name, bitmap);
    }

    outIconShield = *itIconShield;
    return true;
}

QByteArray OsmAnd::MapPresentationEnvironment_P::obtainResourceByName(const QString& name) const
{
    // Try to obtain from external resources first
    if (static_cast<bool>(owner->externalResourcesProvider))
    {
        bool ok = false;
        const auto resource = owner->externalResourcesProvider->getResource(name, owner->displayDensityFactor, &ok);
        if (ok)
            return resource;
    }

    // Otherwise obtain from global
    return getCoreResourcesProvider()->getResource(name, owner->displayDensityFactor);
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
