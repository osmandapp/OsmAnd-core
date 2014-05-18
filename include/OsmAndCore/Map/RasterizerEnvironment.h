#ifndef _OSMAND_CORE_RASTERIZER_ENVIRONMENT_H_
#define _OSMAND_CORE_RASTERIZER_ENVIRONMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IExternalResourcesProvider.h>
#include <OsmAndCore/Map/MapStyle.h>

namespace OsmAnd
{
    class MapStyleValueDefinition;
    class Rasterizer;
    struct MapStyleValue;
    class ObfMapSectionInfo;

    class RasterizerEnvironment_P;
    class OSMAND_CORE_API RasterizerEnvironment
    {
        Q_DISABLE_COPY(RasterizerEnvironment);
    private:
        PrivateImplementation<RasterizerEnvironment_P> _p;
    protected:
    public:
        RasterizerEnvironment(
            const std::shared_ptr<const MapStyle>& style,
            const float displayDensityFactor,
            const QString& localeLanguageId = QLatin1String("en"),
            const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider = nullptr);
        virtual ~RasterizerEnvironment();

        const std::shared_ptr<const MapStyle> style;
        const float displayDensityFactor;
        const QString localeLanguageId;
        const std::shared_ptr<const IExternalResourcesProvider> externalResourcesProvider;

        const std::shared_ptr<const ObfMapSectionInfo>& dummyMapSection;

        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > getSettings() const;
        void setSettings(const QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings);
        void setSettings(const QHash< QString, QString >& newSettings);

    friend class OsmAnd::Rasterizer;
    };
}

#endif // !defined(_OSMAND_CORE_RASTERIZER_ENVIRONMENT_H_)
