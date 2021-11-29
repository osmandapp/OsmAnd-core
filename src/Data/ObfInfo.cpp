#include "ObfInfo.h"

#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfPoiSectionInfo.h"
#include "ObfAddressSectionInfo.h"
#include "ObfTransportSectionInfo.h"
#include <OsmAndCore/Utilities.h>

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

bool OsmAnd::ObfInfo::containsPOIFor(const AreaI pBbox31) const
{
    for (const auto& poiSection : constOf(poiSections))
    {
        bool fitsBBox =
            poiSection->area31.contains(pBbox31) ||
            poiSection->area31.intersects(pBbox31) ||
            pBbox31.contains(poiSection->area31);
        
        bool accept = fitsBBox;
        if (accept)
            return true;
    }

    return false;

    //return containsDataFor(pBbox31, OsmAnd::ZoomLevel::MinZoomLevel, OsmAnd::ZoomLevel::MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::POI));
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
                    poiSection->area31.contains(*pBbox31) ||
                    poiSection->area31.intersects(*pBbox31) ||
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
    
    if (desiredDataTypes.isSet(ObfDataType::Transport))
    {
        for (const auto& transportSection : constOf(transportSections))
        {
            bool accept = true;
            
            // Check by area
            if (pBbox31)
            {
                const auto fitsBBox =
                transportSection->area31.contains(*pBbox31) ||
                transportSection->area31.intersects(*pBbox31) ||
                pBbox31->contains(transportSection->area31);
                
                accept = accept && fitsBBox;
            }
            
            if (accept)
                return true;
        }
    }

    return false;
}

const OsmAnd::Nullable<OsmAnd::LatLon> OsmAnd::ObfInfo::getRegionCenter() const
{
    Nullable<OsmAnd::LatLon> result;
    for (const auto& r : mapSections)
    {
        if (r->calculatedCenter.isSet())
            return result = r->calculatedCenter;
    }
    return result;
}

void OsmAnd::ObfInfo::calculateCenterPointForRegions() const
{
    for (auto reg : addressSections)
    {
        for (const auto& map : mapSections)
        {
            if (reg->name == map->name)
            {
                if (!map->levels.isEmpty())
                {
                    reg->calculatedCenter = map->getCenterLatLon();
                    break;
                }
            }
        }
        if (!reg->calculatedCenter.isSet())
        {
            for (const auto& map : routingSections)
            {
                if (reg->name == map->name)
                {
                    reg->calculatedCenter = OsmAnd::Utilities::convert31ToLatLon(map->area31.center());
                    break;
                }
            }
        }
    }
}

const QStringList OsmAnd::ObfInfo::getRegionNames() const
{
    QStringList rg;
    for (const auto& r : addressSections)
        rg << r->name;
    
    return rg;
}
