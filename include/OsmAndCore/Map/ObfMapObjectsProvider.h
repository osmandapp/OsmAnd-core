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
#include <OsmAndCore/Map/ObfMapObjectsProvider_Metrics.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;

    class ObfMapObjectsProvider_P;
    class OSMAND_CORE_API ObfMapObjectsProvider : public IMapObjectsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapObjectsProvider);
    public:
        enum class Mode
        {
            OnlyBinaryMapObjects,
            OnlyRoads,
            BinaryMapObjectsAndRoads
        };

    private:
        PrivateImplementation<ObfMapObjectsProvider_P> _p;
    protected:
    public:
        ObfMapObjectsProvider(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const Mode mode = Mode::BinaryMapObjectsAndRoads);
        virtual ~ObfMapObjectsProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const Mode mode;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapObjectsProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

        virtual SourceType getSourceType() const;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_OBJECTS_PROVIDER_H_)
