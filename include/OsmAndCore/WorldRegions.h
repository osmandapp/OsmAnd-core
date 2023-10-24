#ifndef _OSMAND_CORE_WORLD_REGIONS_H_
#define _OSMAND_CORE_WORLD_REGIONS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryController.h>

namespace OsmAnd
{
    struct WorldRegion;

    class WorldRegions_P;
    class OSMAND_CORE_API WorldRegions
    {
        Q_DISABLE_COPY_AND_MOVE(WorldRegions)

    public:
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::WorldRegion>& worldRegion)> VisitorFunction;

    private:
        PrivateImplementation<WorldRegions_P> _p;
    protected:
    public:
        WorldRegions(const QString& ocbfFileName);
        virtual ~WorldRegions();

        const QString ocbfFileName;

        static const QString AntarcticaRegionId;
        static const QString AfricaRegionId;
        static const QString AsiaRegionId;
        static const QString AustraliaAndOceaniaRegionId;
        static const QString CentralAmericaRegionId;
        static const QString EuropeRegionId;
        static const QString NorthAmericaRegionId;
        static const QString RussiaRegionId;
        static const QString SouthAmericaRegionId;
        static const QString NauticalRegionId;
        static const QString TravelRegionId;
        static const QString OthersRegionId;

        bool loadWorldRegions(
            QList< std::shared_ptr<const WorldRegion> >* const outRegions,
            const bool keepMapObjects = false,
            const AreaI* const bbox31 = nullptr,
            const VisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_WORLD_REGIONS_H_)
