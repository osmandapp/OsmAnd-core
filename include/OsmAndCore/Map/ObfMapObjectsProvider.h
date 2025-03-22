#ifndef _OSMAND_CORE_OBF_MAP_OBJECTS_PROVIDER_H_
#define _OSMAND_CORE_OBF_MAP_OBJECTS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapObjectsProvider.h>
#include <OsmAndCore/Map/MapStyleEvaluationResult.h>
#include <OsmAndCore/Map/ObfMapObjectsProvider_Metrics.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;

    class ObfMapObjectsProvider_P;
    class OSMAND_CORE_API ObfMapObjectsProvider Q_DECL_FINAL
        : public std::enable_shared_from_this<ObfMapObjectsProvider>
        , public IMapObjectsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapObjectsProvider);
    public:
        enum class Mode
        {
            OnlyBinaryMapObjects,
            OnlyRoads,
            BinaryMapObjectsAndRoads
        };

        enum {
            AddDuplicatedMapObjectsMaxZoom = ZoomLevel14
        };

    private:
        PrivateImplementation<ObfMapObjectsProvider_P> _p;
    protected:
    public:
        ObfMapObjectsProvider(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const Mode mode = Mode::BinaryMapObjectsAndRoads);
        virtual ~ObfMapObjectsProvider();

        std::shared_ptr<const MapPresentationEnvironment> environment;
        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const Mode mode;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;

        bool obtainTiledObfMapObjects(
            const Request& request,
            std::shared_ptr<Data>& outMapObjects,
            ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_OBJECTS_PROVIDER_H_)
