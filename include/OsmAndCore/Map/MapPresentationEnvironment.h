#ifndef _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_H_
#define _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QMap>
#include <QList>
#include <QSet>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/ICoreResourcesProvider.h>
#include <OsmAndCore/Map/IMapStyle.h>
#include <OsmAndCore/Icons.h>

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
        Q_DISABLE_COPY_AND_MOVE(MapPresentationEnvironment);
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
            const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider = nullptr,
            const QSet<QString> disabledAttributes = QSet<QString>());
        virtual ~MapPresentationEnvironment();

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> styleBuiltinValueDefs;

        const std::shared_ptr<const IMapStyle> mapStyle;
        const float displayDensityFactor;
        const float mapScaleFactor;
        const float symbolsScaleFactor;
        const std::shared_ptr<const ICoreResourcesProvider> externalResourcesProvider;
        const QSet<QString> disabledAttributes;

        QString getLocaleLanguageId() const;
        void setLocaleLanguageId(const QString& localeLanguageId);

        LanguagePreference getLanguagePreference() const;
        void setLanguagePreference(const LanguagePreference languagePreference);

        QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue > getSettings() const;
        void setSettings(const QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings);
        void setSettings(const QHash< QString, QString >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;

        std::shared_ptr<const LayeredIconData> getLayeredIconData(
            const QString& tag,
            const QString& value,
            const ZoomLevel zoom,
            const int textLength) const;
        std::shared_ptr<const LayeredIconData> getLayeredIconData(
            const QString& tag,
            const QString& value,
            const ZoomLevel zoom,
            const int textLength,
            const QString& additional) const;
        std::shared_ptr<const IconData> getIconData(const QString& name) const;
        std::shared_ptr<const IconData> getMapIconData(const QString& name) const;
        std::shared_ptr<const IconData> getShaderOrShieldData(const QString& name) const;

        bool obtainIcon(const QString& name, const float scale, sk_sp<const SkImage>& outIcon) const;
        bool obtainMapIcon(const QString& name, const float scale, sk_sp<const SkImage>& outIcon) const;
        bool obtainShaderOrShield(const QString& name, const float scale, sk_sp<const SkImage>& outIcon) const;

        ColorARGB getDefaultBackgroundColor(const ZoomLevel zoom) const;
        void obtainShadowOptions(const ZoomLevel zoom, ShadowMode& mode, ColorARGB& color) const;
        double getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const;
        unsigned int getRoadDensityZoomTile(const ZoomLevel zoom) const;
        unsigned int getRoadsDensityLimitPerTile(const ZoomLevel zoom) const;
        float getDefaultSymbolPathSpacing() const;
        float getDefaultBlockPathSpacing() const;
        float getGlobalPathPadding() const;
        MapStubStyle getDesiredStubsStyle() const;
        QString getWeatherContourLevels(const QString& weatherType, const ZoomLevel zoom) const;
        ColorARGB getTransportRouteColor(const bool nightMode, const QString& renderAttrName) const;
        QHash<QString, int64_t> getLineRenderingAttributes(const QString& renderAttrName) const;
        QHash<QString, int> getGpxColors() const;
        QHash<QString, QList<int>> getGpxWidth() const;
        QPair<QString, uint32_t> getRoadRenderingAttributes(const QString& renderAttrName, const QHash<QString, QString>& additionalSettings) const;

        enum {
            DefaultShadowLevelMin = 0,
            DefaultShadowLevelMax = 256,
        };
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_H_)
