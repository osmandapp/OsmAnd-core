#ifndef _OSMAND_CORE_OBF_DATA_INTERFACE_H_
#define _OSMAND_CORE_OBF_DATA_INTERFACE_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    // Forward declarations
    class ObfsCollection;
    class ObfsCollection_P;
    class ObfReader;
    class ObfFile;
    class ObfMapSectionInfo;
    namespace Model
    {
        class MapObject;
    }
    class IQueryController;
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects;
    }

    class ObfDataInterface_P;
    class OSMAND_CORE_API ObfDataInterface
    {
        Q_DISABLE_COPY(ObfDataInterface);
    private:
        const std::unique_ptr<ObfDataInterface_P> _d;
    protected:
        ObfDataInterface(const QList< std::shared_ptr<ObfReader> >& readers);
    public:
        virtual ~ObfDataInterface();

        void obtainObfFiles(QList< std::shared_ptr<const ObfFile> >* outFiles = nullptr, const IQueryController* const controller = nullptr);
        void obtainBasemapPresenceFlag(bool& basemapPresent, const IQueryController* const controller = nullptr);
        void obtainMapObjects(QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut, MapFoundationType* foundationOut,
            const AreaI& area31, const ZoomLevel zoom,
            const IQueryController* const controller = nullptr, const FilterMapObjectsByIdSignature filterById = nullptr,
            ObfMapSectionReader_Metrics::Metric_loadMapObjects* const metric = nullptr);
        
    friend class OsmAnd::ObfsCollection;
    friend class OsmAnd::ObfsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_DATA_INTERFACE_H_)
