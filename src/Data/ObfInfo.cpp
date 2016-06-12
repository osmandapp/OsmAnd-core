#include "ObfInfo.h"

#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfPoiSectionInfo.h"
#include "ObfAddressSectionInfo.h"

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

bool OsmAnd::ObfInfo::containsPOIFor(const AreaI* const pBbox31) const
{
    return containsDataFor(pBbox31, OsmAnd::ZoomLevel::MinZoomLevel, OsmAnd::ZoomLevel::MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::POI));
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
                bool accept = true;

                // Check by zoom
                accept = accept && (minZoomLevel <= level->maxZoom && level->minZoom <= maxZoomLevel);

                // Check by area
                if (pBbox31)
                {
                    const auto fitsBBox =
                        level->area31.contains(*pBbox31) ||
                        level->area31.intersects(*pBbox31) ||
                        pBbox31->contains(level->area31);

                    accept = accept && fitsBBox;
                }

                if (accept)
                    return true;
            }
        }
    }

    if (desiredDataTypes.isSet(ObfDataType::Routing))
    {
        for (const auto& routingSection : constOf(routingSections))
        {
            bool accept = true;

            // Check by area
            if (pBbox31)
            {
                const auto fitsBBox =
                    routingSection->area31.contains(*pBbox31) ||
                    routingSection->area31.intersects(*pBbox31) ||
                    pBbox31->contains(routingSection->area31);

                accept = accept && fitsBBox;
            }

            if (accept)
                return true;
        }
    }

    if (desiredDataTypes.isSet(ObfDataType::POI))
    {
        for (const auto& poiSection : constOf(poiSections))
        {
            bool accept = true;

            // Check by area
            if (pBbox31)
            {
                const auto fitsBBox =
                    //poiSection->area31.contains(*pBbox31) ||
                    //poiSection->area31.intersects(*pBbox31) ||
                    pBbox31->contains(poiSection->area31);

                accept = accept && fitsBBox;
            }

            if (accept)
                return true;
        }
    }

    if (desiredDataTypes.isSet(ObfDataType::Address))
    {
        for (const auto& addressSection : constOf(addressSections))
        {
            bool accept = true;

            // Check by area
            if (pBbox31)
            {
                const auto fitsBBox =
                    addressSection->area31.contains(*pBbox31) ||
                    addressSection->area31.intersects(*pBbox31) ||
                    pBbox31->contains(addressSection->area31);

                accept = accept && fitsBBox;
            }

            if (accept)
                return true;
        }
    }

    //TODO: ObfTransportSectionInfo

    return false;
}
