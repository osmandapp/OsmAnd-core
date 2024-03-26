#ifndef _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_
#define _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include <QList>
#include <QHash>
#include <QMutex>
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImage.h>
#include <SkPaint.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapStyleConstantValue.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "SkiaUtilities.h"
#include "Icons.h"

namespace OsmAnd
{
    class UnresolvedMapStyle;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapPresentationEnvironment_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapPresentationEnvironment_P);

    public:
        typedef MapPresentationEnvironment::LanguagePreference LanguagePreference;
        typedef MapPresentationEnvironment::ShadowMode ShadowMode;

    private:
        QHash<IMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue> resolveSettings(
            const QHash<QString, QString> &newSettings) const;
    protected:
        MapPresentationEnvironment_P(MapPresentationEnvironment* owner);

        void initialize();

        struct IconsCache
        {
        private:
            mutable QMutex _mutex;
            QHash< QString, sk_sp<const SkImage> > _icons;

            bool obtainIcon(const QString& name, const float scale, const bool colorable, sk_sp<const SkImage>& outIcon)
            {
                QMutexLocker scopedLocker(&_mutex);

                const auto key = makeIconKey(name, scale, colorable);
                auto citIcon = _icons.constFind(key);
                if (citIcon == _icons.cend())
                {
                    QByteArray data;
                    if (!obtainResourceByPath(makeIconPath(name, colorable), data))
                        return false;

                    const auto image = SkiaUtilities::createImageFromVectorData(data, scale * displayDensityFactor);
                    if (!image)
                        return false;

                    citIcon = _icons.insert(key, image);
                }

                outIcon = *citIcon;
                return true;
            }

            bool obtainResourceByPath(const QString& path, QByteArray& outResource) const
            {
                bool ok = false;

                if (externalResourcesProvider)
                {
                    outResource = externalResourcesProvider->containsResource(path, displayDensityFactor)
                        ? externalResourcesProvider->getResource(path, displayDensityFactor, &ok)
                        : externalResourcesProvider->getResource(path, &ok);

                    if (ok)
                        return true;
                }

                outResource = getCoreResourcesProvider()->containsResource(path, displayDensityFactor)
                    ? getCoreResourcesProvider()->getResource(path, displayDensityFactor, &ok)
                    : getCoreResourcesProvider()->getResource(path, &ok);
                if (!ok)
                    LogPrintf(LogSeverityLevel::Warning,
                        "Resource '%s' (requested by MapPresentationEnvironment) was not found", qPrintable(path));

                return ok;
            }

            bool containsResource(const QString& name, bool colorable) const
            {
                const auto resourcePath = makeIconPath(name, colorable);
                bool ok = false;
                ok = ok || externalResourcesProvider && externalResourcesProvider->containsResource(resourcePath, displayDensityFactor);
                ok = ok || externalResourcesProvider && externalResourcesProvider->containsResource(resourcePath);
                ok = ok || getCoreResourcesProvider()->containsResource(resourcePath, displayDensityFactor);
                ok = ok || getCoreResourcesProvider()->containsResource(resourcePath);
                return ok;
            }

            QString makeIconKey(const QString& name, const float scale, const bool colorable) const
            {
                return colorable
                    ? QLatin1String("mc_%1_%2").arg(name).arg(static_cast<int>(scale * 1000))
                    : QLatin1String("%1_%2").arg(name).arg(static_cast<int>(scale * 1000));
            }

            QString makeIconPath(const QString& name, const bool colorable) const
            {
                return colorable
                    ? pathFormat.arg(QLatin1String("mc_%1").arg(name))
                    : pathFormat.arg(name);
            }

        public:
            inline IconsCache(
                const QString& pathFormat_,
                const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider_,
                const float displayDensityFactor_)
                : pathFormat(pathFormat_)
                , externalResourcesProvider(externalResourcesProvider_)
                , displayDensityFactor(displayDensityFactor_)
            {
            }

            const QString pathFormat;
            const std::shared_ptr<const ICoreResourcesProvider> externalResourcesProvider;
            const float displayDensityFactor;

            bool obtainIcon(const QString& name, const float scale, sk_sp<const SkImage>& outIcon)
            {
                if (containsResource(name, false) && obtainIcon(name, scale, false, outIcon))
                    return true;

                if (obtainIcon(name, scale, true, outIcon))
                    return true;

                return false;
            }

            bool obtainResourceByName(const QString& name, QByteArray& outResource, bool& outColorable) const
            {
                if (containsResource(name, true) && obtainResourceByPath(makeIconPath(name, true), outResource))
                {
                    outColorable = true;
                    return true;
                }

                if (obtainResourceByPath(makeIconPath(name, false), outResource))
                {
                    outColorable = false;
                    return true;
                }

                return false;
            }

            bool containsResource(const QString& name) const
            {
                return containsResource(name, false) || containsResource(name, true);
            }
        };

        mutable QReadWriteLock _languagePropertiesLock;
        QString _localeLanguageId;
        LanguagePreference _languagePreference;

        mutable QMutex _settingsChangeMutex;
        QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > _settings;

        std::shared_ptr<const IMapStyle::IAttribute> _defaultBackgroundColorAttribute;
        ColorARGB _defaultBackgroundColor;

        std::shared_ptr<const IMapStyle::IAttribute> _shadowOptionsAttribute;
        ShadowMode _shadowMode;
        ColorARGB _shadowColor;

        std::shared_ptr<const IMapStyle::IAttribute> _polygonMinSizeToDisplayAttribute;
        double _polygonMinSizeToDisplay;

        std::shared_ptr<const IMapStyle::IAttribute> _roadDensityZoomTileAttribute;
        unsigned int _roadDensityZoomTile;

        std::shared_ptr<const IMapStyle::IAttribute> _roadsDensityLimitPerTileAttribute;
        unsigned int _roadsDensityLimitPerTile;

        std::shared_ptr<const IMapStyle::IAttribute> _defaultSymbolPathSpacingAttribute;
        float _defaultSymbolPathSpacing;

        std::shared_ptr<const IMapStyle::IAttribute> _defaultBlockPathSpacingAttribute;
        float _defaultBlockPathSpacing;

        std::shared_ptr<const IMapStyle::IAttribute> _globalPathPaddingAttribute;
        float _globalPathPadding;

        std::shared_ptr<const IMapStyle::IAttribute> _weatherContourLevelsAttribute;

        MapStubStyle _desiredStubsStyle;

        mutable std::shared_ptr<IconsCache> _mapIcons;
        mutable std::shared_ptr<IconsCache> _shadersAndShields;

        bool obtainIconLayerData(
            const std::shared_ptr<const MapObject>& mapObject,
            const MapStyleEvaluationResult& evaluationResult,
            const IMapStyle::ValueDefinitionId valueDefId,
            const float scale,
            IconData& outIconData) const;
        bool obtainResource(const QString& name, QByteArray& outResource) const;

    public:
        virtual ~MapPresentationEnvironment_P();

        ImplementationInterface<MapPresentationEnvironment> owner;

        QString getLocaleLanguageId() const;
        void setLocaleLanguageId(const QString& localeLanguageId);

        LanguagePreference getLanguagePreference() const ;
        void setLanguagePreference(const LanguagePreference languagePreference);

        QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > getSettings() const;
        void setSettings(const QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings);
        
        void setSettings(const QHash< QString, QString >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;
        void applyTo(MapStyleEvaluator &evaluator, const QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > &settings) const;

        LayeredIconData getLayeredIconData(
            const QString& tag,
            const QString& value,
            const ZoomLevel zoom,
            const int textLength,
            const QString* const additional) const;
        IconData getIconData(const QString& name) const;
        IconData getMapIconData(const QString& name) const;
        IconData getShaderOrShieldData(const QString& name) const;

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
        QHash<QString, int> getLineRenderingAttributes(const QString& renderAttrName) const;
        QHash<QString, int> getGpxColors() const;
        QHash<QString, QList<int>> getGpxWidth() const;
        QPair<QString, uint32_t> getRoadRenderingAttributes(const QString& renderAttrName, const QHash<QString, QString>& additionalSettings) const;

    friend class OsmAnd::MapPresentationEnvironment;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_)
