#ifndef _OSMAND_CORE_WORLD_REGIONS_P_H_
#define _OSMAND_CORE_WORLD_REGIONS_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QHash>
#include <QString>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IQueryController.h"
#include "WorldRegions.h"

namespace OsmAnd
{
    class BinaryMapObject;
    class WorldRegions;
    class WorldRegions_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(WorldRegions_P)

    public:
        typedef WorldRegions::VisitorFunction VisitorFunction;

    private:
        void addPolygonToRegionIfValid(const std::shared_ptr<const OsmAnd::BinaryMapObject>& mapObject, const std::shared_ptr<WorldRegion> &worldRegion) const;
    protected:
        WorldRegions_P(WorldRegions* const owner);
    public:
        ~WorldRegions_P();

        ImplementationInterface<WorldRegions> owner;
        
        void extracted(unsigned int downloadNameAttributeId, bool keepMapObjects, unsigned int keyNameAttributeId, unsigned int langAttributeId, unsigned int leftHandDrivingAttributeId, const std::shared_ptr<const OsmAnd::BinaryMapObject> &mapObject, unsigned int metricAttributeId, unsigned int populationAttributeId, unsigned int regionFullNameAttributeId, unsigned int regionParentNameAttributeId, unsigned int roadSignsAttributeId, unsigned int wikiLinkAttributeId, const std::shared_ptr<OsmAnd::WorldRegion> &worldRegion) const;
        
        bool loadWorldRegions(
            QList< std::shared_ptr<const WorldRegion> >* const outRegions,
            const bool keepMapObjects,
            const AreaI* const bbox31,
            const VisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController) const;

    friend class OsmAnd::WorldRegions;
    };
}

#endif // !defined(_OSMAND_CORE_WORLD_REGIONS_P_H_)
