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
    class WorldRegions_P;
    class OSMAND_CORE_API WorldRegions
    {
        Q_DISABLE_COPY(WorldRegions)
    public:
        class OSMAND_CORE_API WorldRegion
        {
            Q_DISABLE_COPY(WorldRegion)
        private:
        protected:
            WorldRegion(
                const QString& id,
                const QString& name,
                const QHash<QString, QString>& localizedNames,
                const QString& parentId = QString::null);
        public:
            virtual ~WorldRegion();

            const QString parentId;

            const QString id;
            const QString name;
            const QHash<QString, QString> localizedNames;

        friend class OsmAnd::WorldRegions_P;
        friend class OsmAnd::WorldRegions;
        };

    private:
        PrivateImplementation<WorldRegions_P> _p;
    protected:
    public:
        WorldRegions(const QString& ocbfFileName);
        virtual ~WorldRegions();

        const QString ocbfFileName;

        static const QString AfricaRegionId;
        static const QString AsiaRegionId;
        static const QString AustraliaAndOceaniaRegionId;
        static const QString CentralAmericaRegionId;
        static const QString EuropeRegionId;
        static const QString NorthAmericaRegionId;
        static const QString SouthAmericaRegionId;

        bool loadWorldRegions(
            QHash< QString, std::shared_ptr<const WorldRegion> >& outRegions,
            const IQueryController* const controller = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_WORLD_REGIONS_H_)
