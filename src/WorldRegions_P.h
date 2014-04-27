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
    class WorldRegions;
    class WorldRegions_P
    {
        Q_DISABLE_COPY(WorldRegions_P)
    public:
        typedef WorldRegions::WorldRegion WorldRegion;

    private:
    protected:
        WorldRegions_P(WorldRegions* const owner);
    public:
        virtual ~WorldRegions_P();

        ImplementationInterface<WorldRegions> owner;
        
        bool loadWorldRegions(
            QHash< QString, std::shared_ptr<const WorldRegion> >& outRegions,
            const IQueryController* const controller) const;

    friend class OsmAnd::WorldRegions;
    };
}

#endif // !defined(_OSMAND_CORE_WORLD_REGIONS_P_H_)
