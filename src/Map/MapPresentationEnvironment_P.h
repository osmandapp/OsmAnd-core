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
#include "Icons.h"
#include "IconsProvider.h"

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

        std::shared_ptr<const IMapStyle::IAttribute> _base3DBuildingsColorAttribute;
        std::shared_ptr<const IMapStyle::IAttribute> _3DBuildingsAlphaAttribute;
        std::shared_ptr<const IMapStyle::IAttribute> _useDefaultBuildingColorAttribute;

        float _default3DBuildingHeight;
        float _default3DBuildingLevelHeight;

        MapStubStyle _desiredStubsStyle;

        std::shared_ptr<IconsProvider> _mapIcons;
        std::shared_ptr<IconsProvider> _shadersAndShields;

        bool obtainIconLayerData(
            const std::shared_ptr<const MapObject>& mapObject,
            const MapStyleEvaluationResult& evaluationResult,
            const IMapStyle::ValueDefinitionId valueDefId,
            const float scale,
            std::shared_ptr<const IconData>& outIconData) const;
        bool obtainResource(const QString& name, QByteArray& outResource) const;
        std::shared_ptr<const IconData> getIconDataFromProvider(const QString& name, const std::shared_ptr<const IconsProvider>& provider) const;

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

        std::shared_ptr<const LayeredIconData> getLayeredIconData(
            const QString& tag,
            const QString& value,
            const ZoomLevel zoom,
            const int textLength,
            const QString* const additional) const;
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
        QHash<QString, int> getLineRenderingAttributes(const QString& renderAttrName) const;
        QHash<QString, int> getGpxColors() const;
        QHash<QString, QList<int>> getGpxWidth() const;
        QPair<QString, uint32_t> getRoadRenderingAttributes(const QString& renderAttrName, const QHash<QString, QString>& additionalSettings) const;
        FColorARGB get3DBuildingsColor() const;
        float getDefault3DBuildingHeight() const;
        float getDefault3DBuildingLevelHeight() const;
        bool getUseDefaultBuildingColor() const;
        bool getShow3DBuildingParts() const;

    friend class OsmAnd::MapPresentationEnvironment;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_)
