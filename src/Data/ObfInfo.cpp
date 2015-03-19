#include "ObfInfo.h"

#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfPoiSectionInfo.h"

OsmAnd::ObfInfo::ObfInfo()
    : version(-1)
    , creationTimestamp(0)
    , isBasemap(false)
    , isBasemapWithCoastlines(false)
{
}

OsmAnd::ObfInfo::~ObfInfo()
{
}

bool OsmAnd::ObfInfo::containsDataFor(
    const AreaI* const pBbox31,
    const ZoomLevel minZoomLevel,
    const ZoomLevel maxZoomLevel,
    const ObfDataTypesMask desiredDataTypes) const
{
    if (desiredDataTypes.isSet(ObfDataType::Map))
    {
        for (const auto& mapSection : constOf(mapSections))
        {
            for (const auto& level : constOf(mapSection->levels))
            {
                bool accept = false;

                // Check by zoom
                accept = accept || (minZoomLevel <= level->maxZoom && level->minZoom <= maxZoomLevel);

                // Check by area
                if (pBbox31)
                {
                    accept = accept || level->area31.contains(*pBbox31);
                    accept = accept || level->area31.intersects(*pBbox31);
                    accept = accept || pBbox31->contains(level->area31);
                }

                if (accept)
                    return true;
            }
        }
    }

    if (desiredDataTypes.isSet(ObfDataType::Routing))
    {
        // Routing sections do not contain areas that are suitable for this check,
        // so in case there are no map sections but there are routing sections, lie that this file surely has data
        if (!routingSections.isEmpty() && mapSections.isEmpty())
            return true;
    }

    if (desiredDataTypes.isSet(ObfDataType::POI))
    {
        for (const auto& poiSection : constOf(poiSections))
        {
            bool accept = false;

            // Check by area
            if (pBbox31)
            {
                accept = accept || poiSection->area31.contains(*pBbox31);
                accept = accept || poiSection->area31.intersects(*pBbox31);
                accept = accept || pBbox31->contains(poiSection->area31);
            }

            if (accept)
                return true;
        }
    }

    //TODO: ObfAddressSectionInfo
    //TODO: ObfTransportSectionInfo

    return false;
}
