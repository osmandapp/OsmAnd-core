#include "MapPresentationEnvironment_P.h"
#include "MapPresentationEnvironment.h"

#include <limits>

#include "QtCommon.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include "restore_internal_warnings.h"

#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleConstantValue.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "ObfMapSectionInfo.h"
#include "CoreResourcesEmbeddedBundle.h"
#include "ICoreResourcesProvider.h"
#include "QKeyValueIterator.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapPresentationEnvironment_P::MapPresentationEnvironment_P(MapPresentationEnvironment* owner_)
    : owner(owner_)
    , _localeLanguageId(QLatin1String("en"))
    , _languagePreference(LanguagePreference::LocalizedOrNative)
{
}

OsmAnd::MapPresentationEnvironment_P::~MapPresentationEnvironment_P()
{
}

void OsmAnd::MapPresentationEnvironment_P::initialize()
{
    _defaultBackgroundColorAttribute = owner->mapStyle->getAttribute(QLatin1String("defaultColor"));
    _defaultBackgroundColor = ColorRGB(0xf1, 0xee, 0xe8);

    _shadowOptionsAttribute = owner->mapStyle->getAttribute(QLatin1String("shadowRendering"));
    _shadowMode = ShadowMode::NoShadow;
    _shadowColor = ColorRGB(0x96, 0x96, 0x96);

    _polygonMinSizeToDisplayAttribute = owner->mapStyle->getAttribute(QLatin1String("polygonMinSizeToDisplay"));
    _polygonMinSizeToDisplay = 0.0;

    _roadDensityZoomTileAttribute = owner->mapStyle->getAttribute(QLatin1String("roadDensityZoomTile"));
    _roadDensityZoomTile = 0;

    _roadsDensityLimitPerTileAttribute = owner->mapStyle->getAttribute(QLatin1String("roadsDensityLimitPerTile"));
    _roadsDensityLimitPerTile = 0;

    _defaultSymbolPathSpacingAttribute = owner->mapStyle->getAttribute(QLatin1String("defaultSymbolPathSpacing"));
    _defaultSymbolPathSpacing = 0.0f;

    _defaultBlockPathSpacingAttribute = owner->mapStyle->getAttribute(QLatin1String("defaultBlockPathSpacing"));
    _defaultBlockPathSpacing = 0.0f;

    _globalPathPaddingAttribute = owner->mapStyle->getAttribute(QLatin1String("globalPathPadding"));
    _globalPathPadding = 0.0f;

    _weatherContourLevelsAttribute = owner->mapStyle->getAttribute(QLatin1String("weatherContourLevels"));

    _desiredStubsStyle = MapStubStyle::Unspecified;
}

QString OsmAnd::MapPresentationEnvironment_P::getLocaleLanguageId() const
{
    QReadLocker scopedLocker(&_languagePropertiesLock);

    return _localeLanguageId;
}

void OsmAnd::MapPresentationEnvironment_P::setLocaleLanguageId(const QString& localeLanguageId)
{
    QWriteLocker scopedLocker(&_languagePropertiesLock);

    _localeLanguageId = localeLanguageId;
}

OsmAnd::MapPresentationEnvironment::LanguagePreference OsmAnd::MapPresentationEnvironment_P::getLanguagePreference() const
{
    QReadLocker scopedLocker(&_languagePropertiesLock);

    return _languagePreference;
}

void OsmAnd::MapPresentationEnvironment_P::setLanguagePreference(const LanguagePreference languagePreference)
{
    QWriteLocker scopedLocker(&_languagePropertiesLock);

    _languagePreference = languagePreference;
}

QHash< OsmAnd::IMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue > OsmAnd::MapPresentationEnvironment_P::getSettings() const
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    return detachedOf(_settings);
}

void OsmAnd::MapPresentationEnvironment_P::setSettings(const QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings)
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    _settings = newSettings;
}

QHash<OsmAnd::IMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue> OsmAnd::MapPresentationEnvironment_P::resolveSettings(const QHash<QString, QString> &newSettings) const
{
    QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > resolvedSettings;
    resolvedSettings.reserve(newSettings.size());
    
    for (const auto& itSetting : rangeOf(newSettings))
    {
        const auto& name = itSetting.key();
        const auto& value = itSetting.value();
        
        // Resolve input-value definition by name
        const auto valueDefId = owner->mapStyle->getValueDefinitionIdByName(name);
        const auto& valueDef = owner->mapStyle->getValueDefinitionById(valueDefId);
        if (!valueDef || valueDef->valueClass != MapStyleValueDefinition::Class::Input)
        {
            LogPrintf(LogSeverityLevel::Warning,
                      "Setting of '%s' to '%s' impossible: failed to resolve input value definition failed with such name",
                      qPrintable(name),
                      qPrintable(value));
            continue;
        }
        
        // Parse value
        MapStyleConstantValue parsedValue;
        if (!owner->mapStyle->parseValue(value, valueDef, parsedValue))
        {
            LogPrintf(LogSeverityLevel::Warning,
                      "Setting of '%s' to '%s' impossible: failed to parse value",
                      qPrintable(name),
                      qPrintable(value));
            continue;
        }
        
        resolvedSettings.insert(valueDefId, parsedValue);
    }
    return resolvedSettings;
}

void OsmAnd::MapPresentationEnvironment_P::setSettings(const QHash< QString, QString >& newSettings)
{
    QHash<IMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue> resolvedSettings = resolveSettings(newSettings);
    
    // Special case for night mode
    if (resolvedSettings.contains(MapStyleBuiltinValueDefinitions::get()->id_INPUT_NIGHT_MODE))
    {
        _desiredStubsStyle = (resolvedSettings[MapStyleBuiltinValueDefinitions::get()->id_INPUT_NIGHT_MODE].asSimple.asInt == 1)
        ? MapStubStyle::Dark
        : MapStubStyle::Light;
    }
    else
    {
        _desiredStubsStyle = MapStubStyle::Light;
    }

    setSettings(resolvedSettings);
}

void OsmAnd::MapPresentationEnvironment_P::applyTo(MapStyleEvaluator &evaluator, const QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > &settings) const
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    for (const auto& settingEntry : rangeOf(constOf(settings)))
    {
        const auto& valueDefId = settingEntry.key();
        const auto& settingValue = settingEntry.value();

        const auto& valueDef = owner->mapStyle->getValueDefinitionById(valueDefId);
        if (!valueDef)
            continue;

        switch (valueDef->dataType)
        {
            case MapStyleValueDataType::Integer:
                evaluator.setIntegerValue(valueDefId,
                    settingValue.isComplex
                    ? settingValue.asComplex.asInt.evaluate(owner->displayDensityFactor)
                    : settingValue.asSimple.asInt);
                break;
            case MapStyleValueDataType::Float:
                evaluator.setFloatValue(valueDefId,
                    settingValue.isComplex
                    ? settingValue.asComplex.asFloat.evaluate(owner->displayDensityFactor)
                    : settingValue.asSimple.asFloat);
                break;
            case MapStyleValueDataType::Boolean:
            case MapStyleValueDataType::String:
            case MapStyleValueDataType::Color:
                assert(!settingValue.isComplex);
                evaluator.setIntegerValue(valueDefId, settingValue.asSimple.asUInt);
                break;
        }
    }
}

void OsmAnd::MapPresentationEnvironment_P::applyTo(MapStyleEvaluator& evaluator) const
{
    applyTo(evaluator, _settings);
}

bool OsmAnd::MapPresentationEnvironment_P::obtainIcon(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outIcon) const
{
    if (containsResourceByName(getShaderResourcePath(name)))
        return obtainShader(name, scale, outIcon);
    
    if (containsResourceByName(getShieldResourcePath(name)))
        return obtainTextShield(name, scale, outIcon);

    if (containsResourceByName(getMapIconResourcePath(name)))
        return obtainMapIcon(name, scale, outIcon);

    LogPrintf(LogSeverityLevel::Warning, "Resource '%s' (requested by MapPresentationEnvironment) was not found", qPrintable(name));

    return false;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainShader(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outShader) const
{
    QMutexLocker scopedLocker(&_shadersMutex);

    const auto key = makeIconKey(name, scale);
    auto itShader = _shaders.constFind(key);
    if (itShader == _shaders.cend())
    {
        // Get data from embedded resources
        const auto data = obtainResourceByName(getShaderResourcePath(name));
        
        // Decode bitmap for a shader
        const auto image = SkiaUtilities::createImageFromVectorData(data, scale * owner->displayDensityFactor);
        if (!image)
        {
            return false;
        }

        itShader = _shaders.insert(key, image);
    }

    // Create shader from that bitmap
    outShader = *itShader;
    return true;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainMapIcon(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outIcon) const
{
    QMutexLocker scopedLocker(&_mapIconsMutex);

    const auto key = makeIconKey(name, scale);
    auto itIcon = _mapIcons.constFind(key);
    if (itIcon == _mapIcons.cend())
    {
        // Get data from embedded resources
        auto data = obtainResourceByName(getMapIconResourcePath(name));
        
        // Decode bitmap for a shader
        const auto image = SkiaUtilities::createImageFromVectorData(data, scale * owner->displayDensityFactor);
        if (!image)
        {
            return false;
        }

        itIcon = _mapIcons.insert(key, image);
    }

    outIcon = *itIcon;
    return true;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainTextShield(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outTextShield) const
{
    QMutexLocker scopedLocker(&_textShieldsMutex);

    const auto key = makeIconKey(name, scale);
    auto itTextShield = _textShields.constFind(key);
    if (itTextShield == _textShields.cend())
    {
        // Get data from embedded resources
        auto data = obtainResourceByName(getShieldResourcePath(name));

        // Decode bitmap for a shader
        const auto image = SkiaUtilities::createImageFromVectorData(data, scale * owner->displayDensityFactor);
        if (!image)
        {
            return false;
        }

        itTextShield = _textShields.insert(key, image);
    }

    outTextShield = *itTextShield;
    return true;
}

bool OsmAnd::MapPresentationEnvironment_P::obtainIconShield(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outIconShield) const
{
    QMutexLocker scopedLocker(&_iconShieldsMutex);

    const auto key = makeIconKey(name, scale);
    auto itIconShield = _iconShields.constFind(key);
    if (itIconShield == _iconShields.cend())
    {
        // Get data from embedded resources
        auto data = obtainResourceByName(getShieldResourcePath(name));

        // Decode bitmap for a shader
        const auto image = SkiaUtilities::createImageFromVectorData(data, scale * owner->displayDensityFactor);
        if (!image)
        {
            return false;
        }

        itIconShield = _iconShields.insert(key, image);
    }

    outIconShield = *itIconShield;
    return true;
}

QString OsmAnd::MapPresentationEnvironment_P::getShaderResourcePath(const QString& name) const
{
    return QString::fromLatin1("map/shaders/%1.svg").arg(name);
}

QString OsmAnd::MapPresentationEnvironment_P::getMapIconResourcePath(const QString& name) const
{
    return QString::fromLatin1("map/icons/%1.svg").arg(name);
}

QString OsmAnd::MapPresentationEnvironment_P::getShieldResourcePath(const QString& name) const
{
    return QString::fromLatin1("map/shields/%1.svg").arg(name);
}

bool OsmAnd::MapPresentationEnvironment_P::containsResourceByName(const QString& name) const
{
    bool ok = false;
    ok = ok || owner->externalResourcesProvider && owner->externalResourcesProvider->containsResource(name, owner->displayDensityFactor);
    ok = ok || owner->externalResourcesProvider && owner->externalResourcesProvider->containsResource(name);
    ok = ok || getCoreResourcesProvider()->containsResource(name, owner->displayDensityFactor);
    ok = ok || getCoreResourcesProvider()->containsResource(name);
    return ok;
}

QByteArray OsmAnd::MapPresentationEnvironment_P::obtainResourceByName(const QString& name) const
{
    bool ok = false;

    // Try to obtain from external resources first
    if (static_cast<bool>(owner->externalResourcesProvider))
    {
        const auto resource = owner->externalResourcesProvider->containsResource(name, owner->displayDensityFactor)
            ? owner->externalResourcesProvider->getResource(name, owner->displayDensityFactor, &ok)
            : owner->externalResourcesProvider->getResource(name, &ok);
        if (ok)
            return resource;
    }

    // Otherwise obtain from global
    const auto resource = getCoreResourcesProvider()->containsResource(name, owner->displayDensityFactor)
        ? getCoreResourcesProvider()->getResource(name, owner->displayDensityFactor, &ok)
        : getCoreResourcesProvider()->getResource(name, &ok);
    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Warning,
            "Resource '%s' (requested by MapPresentationEnvironment) was not found",
            qPrintable(name));
        return QByteArray();
    }
    return resource;
}

OsmAnd::ColorARGB OsmAnd::MapPresentationEnvironment_P::getDefaultBackgroundColor(const ZoomLevel zoom) const
{
    auto result = _defaultBackgroundColor;

    if (_defaultBackgroundColorAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_defaultBackgroundColorAttribute, &evalResult))
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, result.argb);
    }

    return result;
}

void OsmAnd::MapPresentationEnvironment_P::obtainShadowOptions(const ZoomLevel zoom, ShadowMode& mode, ColorARGB& color) const
{
    bool ok;
    mode = _shadowMode;
    color = _shadowColor;

    if (_shadowOptionsAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_shadowOptionsAttribute, &evalResult))
        {
            int modeValue = 0;
            ok = evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, modeValue);
            if (ok)
                mode = static_cast<ShadowMode>(modeValue);

            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, color.argb);
        }
    }
}

double OsmAnd::MapPresentationEnvironment_P::getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const
{
    auto result = _polygonMinSizeToDisplay;

    if (_polygonMinSizeToDisplayAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_polygonMinSizeToDisplayAttribute, &evalResult))
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

    if (_roadDensityZoomTileAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_roadDensityZoomTileAttribute, &evalResult))
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, result);
    }

    return result;
}

unsigned int OsmAnd::MapPresentationEnvironment_P::getRoadsDensityLimitPerTile(const ZoomLevel zoom) const
{
    auto result = _roadsDensityLimitPerTile;

    if (_roadsDensityLimitPerTileAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_roadsDensityLimitPerTileAttribute, &evalResult))
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, result);
    }

    return result;
}

float OsmAnd::MapPresentationEnvironment_P::getDefaultSymbolPathSpacing() const
{
    auto result = _defaultSymbolPathSpacing;

    if (_defaultSymbolPathSpacingAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->symbolsScaleFactor);
        applyTo(evaluator);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_defaultSymbolPathSpacingAttribute, &evalResult))
        {
            float value = 0.0f;
            if (evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_FLOAT_VALUE, value))
                result = value;
        }
    }

    return result;
}

float OsmAnd::MapPresentationEnvironment_P::getDefaultBlockPathSpacing() const
{
    auto result = _defaultBlockPathSpacing;

    if (_defaultBlockPathSpacingAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->symbolsScaleFactor);
        applyTo(evaluator);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_defaultBlockPathSpacingAttribute, &evalResult))
        {
            float value = 0.0f;
            if (evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_FLOAT_VALUE, value))
                result = value;
        }
    }

    return result;
}

float OsmAnd::MapPresentationEnvironment_P::getGlobalPathPadding() const
{
    auto result = _globalPathPadding;

    if (_globalPathPaddingAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->symbolsScaleFactor);
        applyTo(evaluator);

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_globalPathPaddingAttribute, &evalResult))
        {
            float value = 0.0f;
            if (evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_FLOAT_VALUE, value))
                result = value;
        }
    }

    return result;
}

OsmAnd::MapStubStyle OsmAnd::MapPresentationEnvironment_P::getDesiredStubsStyle() const
{
    return _desiredStubsStyle;
}

QString OsmAnd::MapPresentationEnvironment_P::getWeatherContourLevels(const QString& weatherType, const ZoomLevel zoom) const
{
    QString result;
    if (_weatherContourLevelsAttribute)
    {
        MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
        applyTo(evaluator);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
        evaluator.setIntegerValue(owner->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);
        evaluator.setStringValue(owner->styleBuiltinValueDefs->id_INPUT_ADDITIONAL, QString::asprintf("weatherType=%s", qPrintable(weatherType)));

        MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
        if (evaluator.evaluate(_weatherContourLevelsAttribute, &evalResult))
            evalResult.getStringValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_STRING_VALUE, result);
    }
    return result;
}

OsmAnd::ColorARGB OsmAnd::MapPresentationEnvironment_P::getTransportRouteColor(const bool nightMode, const QString& renderAttrName) const
{
    ColorARGB result = ColorRGB(0xff, 0x0, 0x0);

    MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
    applyTo(evaluator);
    evaluator.setBooleanValue(owner->styleBuiltinValueDefs->id_INPUT_NIGHT_MODE, nightMode);
    
    auto renderAttr = owner->mapStyle->getAttribute(renderAttrName);
    
    if (renderAttr == nullptr)
        return result;
    MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());
    if (evaluator.evaluate(renderAttr, &evalResult))
        evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, result.argb);
    
    return result;
}

QPair<QString, uint32_t> OsmAnd::MapPresentationEnvironment_P::getRoadRenderingAttributes(const QString& renderAttrName, const QHash<QString, QString>& additionalSettings) const
{
    QString name = QStringLiteral("undefined");
    uint32_t argb = 0xFFFFFFFF;
    
    MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
    applyTo(evaluator);
    const auto& resolvedAdditionalSettings = resolveSettings(additionalSettings);
    applyTo(evaluator, resolvedAdditionalSettings);
    
    auto renderAttr = owner->mapStyle->getAttribute(renderAttrName);
    MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());

    if (renderAttr)
    {
        if (evaluator.evaluate(renderAttr, &evalResult))
        {
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, argb);
            evalResult.getStringValue(owner->styleBuiltinValueDefs->id_OUTPUT_ATTR_STRING_VALUE, name);
        }
    }
    
    return QPair<QString, uint32_t>(name, argb);
}

QHash<QString, int> OsmAnd::MapPresentationEnvironment_P::getLineRenderingAttributes(const QString& renderAttrName) const
{
    int color = -1, shadowColor = -1, color_2 = -1, color_3 = -1;
    float strokeWidth = -1.0f, strokeWidth_2 = -1.0f, strokeWidth_3 = -1.0f, shadowRadius = -1.0f, strokeWidth_0 = -1.0f;
    
    MapStyleEvaluator evaluator(owner->mapStyle, owner->displayDensityFactor * owner->mapScaleFactor);
    applyTo(evaluator);
    auto renderAttr = owner->mapStyle->getAttribute(renderAttrName);
    MapStyleEvaluationResult evalResult(owner->mapStyle->getValueDefinitionsCount());

    if (renderAttr)
    {
        if (evaluator.evaluate(renderAttr, &evalResult))
        {
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_COLOR, color);
            evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH, strokeWidth);
            evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
            evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_2, strokeWidth_2);
            evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_0, strokeWidth_0);
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_COLOR_2, color_2);
            evalResult.getFloatValue(owner->styleBuiltinValueDefs->id_OUTPUT_STROKE_WIDTH_3, strokeWidth_3);
            evalResult.getIntegerValue(owner->styleBuiltinValueDefs->id_OUTPUT_COLOR_3, color_3);
        }
    }
    QHash<QString, int> map;
    map.insert(QStringLiteral("color"), color);
    map.insert(QStringLiteral("strokeWidth"), strokeWidth);
    map.insert(QStringLiteral("shadowRadius"), shadowRadius);
    map.insert(QStringLiteral("shadowColor"), shadowColor);
    map.insert(QStringLiteral("strokeWidth_2"), strokeWidth_2);
    map.insert(QStringLiteral("strokeWidth_0"), strokeWidth_0);
    map.insert(QStringLiteral("color_2"), color_2);
    map.insert(QStringLiteral("strokeWidth_3"), strokeWidth_3);
    map.insert(QStringLiteral("color_3"), color_3);

    return map;
}

QHash<QString, int> OsmAnd::MapPresentationEnvironment_P::getGpxColors() const
{
    QHash<QString, int> result;
    const auto &renderAttr = owner->mapStyle->getAttribute(QStringLiteral("gpx"));
    const auto &ruleNode = renderAttr->getRootNodeRef()->getOneOfConditionalSubnodesRef();
    if (ruleNode.isEmpty())
        return result;

    const auto &switchNode = ruleNode.first();
    const auto &conditionals = switchNode->getOneOfConditionalSubnodesRef();
    for (const auto& conditional : constOf(conditionals))
    {
        const auto& ruleNodeValues = conditional->getValuesRef();
        QString colorName = QStringLiteral("");
        int colorValue = 0;
        for (const auto &ruleValueEntry : rangeOf(constOf(ruleNodeValues)))
        {
            const auto valueDefId = ruleValueEntry.key();
            const auto& valueDef = owner->mapStyle->getValueDefinitionRefById(valueDefId);
            if (valueDef->dataType == MapStyleValueDataType::String)
                colorName = owner->mapStyle->getStringById(ruleValueEntry.value().asConstantValue.asSimple.asUInt);
            else if (valueDef->dataType == MapStyleValueDataType::Color)
                colorValue = ruleValueEntry.value().asConstantValue.asSimple.asInt;
                
        }
        if (!colorName.isEmpty() && colorValue != 0)
            result.insert(colorName, colorValue);
    }
    return result;
}

QHash<QString, QList<int>> OsmAnd::MapPresentationEnvironment_P::getGpxWidth() const
{
    QHash<QString, QList<int>> result;
    const auto &renderAttr = owner->mapStyle->getAttribute(QStringLiteral("gpx"));
    const auto &ruleNode = renderAttr->getRootNodeRef()->getOneOfConditionalSubnodesRef();
    if (ruleNode.isEmpty())
        return result;

    const auto &switchNode = ruleNode.first();
    const auto &applyNode = switchNode->getApplySubnodesRef().first();
    const auto &conditionals = applyNode->getOneOfConditionalSubnodesRef();

    for (const auto& conditional : constOf(conditionals))
    {
        const auto& ruleNodeValues = conditional->getValuesRef();
        QString widthName = QStringLiteral("");
        int widthNameStep = 1;

        for (const auto &ruleValueEntry : rangeOf(constOf(ruleNodeValues)))
        {
            const auto valueDefId = ruleValueEntry.key();
            const auto& valueDef = owner->mapStyle->getValueDefinitionRefById(valueDefId);
            if (valueDef->dataType == MapStyleValueDataType::String)
                widthName = owner->mapStyle->getStringById(ruleValueEntry.value().asConstantValue.asSimple.asUInt);
        }

        const auto &applyIfNodes = conditional->getApplySubnodesRef();
        for (const auto& conditionalIf : constOf(applyIfNodes))
        {
            int minZoomValue = 0;
            int maxZoomValue = -1;
            int strokeWidthValue = 1;

            const auto& ruleNodeIfValues = conditionalIf->getValuesRef();
            for (const auto &ruleValueIfEntry : rangeOf(constOf(ruleNodeIfValues)))
            {
                const auto valueDefIfId = ruleValueIfEntry.key();
                const auto& valueDefIf = owner->mapStyle->getValueDefinitionRefById(valueDefIfId);
                if (valueDefIf->dataType == MapStyleValueDataType::Integer)
                {
                    const auto valueInt = ruleValueIfEntry.value().asConstantValue.asSimple.asInt;
                    if (minZoomValue < valueInt)
                    {
                        if (minZoomValue > 0)
                            maxZoomValue = valueInt;
                        else
                            minZoomValue = valueInt;
                    }
                    else
                    {
                        maxZoomValue = minZoomValue;
                        minZoomValue = valueInt;
                    }
                }
                else if (valueDefIf->dataType == MapStyleValueDataType::Float)
                {
                    strokeWidthValue = ruleValueIfEntry.value().asConstantValue.asSimple.asFloat;
                }
            }

            if (!widthName.isEmpty() && minZoomValue > 0)
                result.insert(QStringLiteral("%1_%2").arg(widthName).arg(widthNameStep++), QList<int> {minZoomValue, maxZoomValue, strokeWidthValue});
        }
    }
    return result;
}
