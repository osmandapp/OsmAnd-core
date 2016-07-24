#ifndef _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_H_
#define _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QMap>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/ICoreResourcesProvider.h>
#include <OsmAndCore/Map/IMapStyle.h>

class SkBitmap;

namespace OsmAnd
{
    class MapStyleEvaluator;
    class MapStyleValueDefinition;
    class MapStyleBuiltinValueDefinitions;
    struct MapStyleConstantValue;
    class ObfMapSectionInfo;

    class MapPresentationEnvironment_P;
    class OSMAND_CORE_API MapPresentationEnvironment
    {
        Q_DISABLE_COPY_AND_MOVE(MapPresentationEnvironment)
    public:
        enum class LanguagePreference
        {
            // "name" only
            NativeOnly,

            // "name:$locale" or "name"
            LocalizedOrNative,

            // "name" and "name:$locale"
            NativeAndLocalized,

            // "name" and ( "name:$locale" or transliterate("name") )
            NativeAndLocalizedOrTransliterated,

            // "name:$locale" and "name"
            LocalizedAndNative,

            // ( "name:$locale" or transliterate("name") ) and "name"
            LocalizedOrTransliteratedAndNative,

            // ( "name:$locale" or transliterate("name") )
            LocalizedOrTransliterated,
        };

        enum class ShadowMode
        {
            NoShadow = 0,
            OneStep = 1,
            BlurShadow = 2,
            SolidShadow = 3
        };

    private:
        PrivateImplementation<MapPresentationEnvironment_P> _p;
    protected:
    public:
        MapPresentationEnvironment(
            const std::shared_ptr<const IMapStyle>& mapStyle,
            const float displayDensityFactor = 1.0f,
            const float mapScaleFactor = 1.0f,
            const float symbolsScaleFactor = 1.0f,
            const QString& localeLanguageId = QLatin1String("en"),
            const LanguagePreference languagePreference = LanguagePreference::LocalizedOrNative,
            const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider = nullptr);
        virtual ~MapPresentationEnvironment();

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> styleBuiltinValueDefs;

        const std::shared_ptr<const IMapStyle> mapStyle;
        const float displayDensityFactor;
        const float mapScaleFactor;
        const float symbolsScaleFactor;
        const QString localeLanguageId;
        const LanguagePreference languagePreference;
        const std::shared_ptr<const ICoreResourcesProvider> externalResourcesProvider;

        QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue > getSettings() const;
        void setSettings(const QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings);
        void setSettings(const QHash< QString, QString >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;

        bool obtainShaderBitmap(const QString& name, std::shared_ptr<const SkBitmap>& outShaderBitmap) const;
        bool obtainMapIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;
        bool obtainTextShield(const QString& name, std::shared_ptr<const SkBitmap>& outTextShield) const;
        bool obtainIconShield(const QString& name, std::shared_ptr<const SkBitmap>& outIconShield) const;

        ColorARGB getDefaultBackgroundColor(const ZoomLevel zoom) const;
        void obtainShadowOptions(const ZoomLevel zoom, ShadowMode& mode, ColorARGB& color) const;
        double getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const;
        unsigned int getRoadDensityZoomTile(const ZoomLevel zoom) const;
        unsigned int getRoadsDensityLimitPerTile(const ZoomLevel zoom) const;
        float getDefaultSymbolPathSpacing() const;
        float getDefaultBlockPathSpacing() const;
        float getGlobalPathPadding() const;
        MapStubStyle getDesiredStubsStyle() const;

        enum {
            DefaultShadowLevelMin = 0,
            DefaultShadowLevelMax = 256,
        };
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_H_)
