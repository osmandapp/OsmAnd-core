#include "ObfInfo.h"

#include "ObfMapSectionInfo.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfPoiSectionInfo.h"

OsmAnd::ObfInfo::ObfInfo()
    : version(-1)
    , creationTimestamp(0)
    , isBasemap(false)
{
}

OsmAnd::ObfInfo::~ObfInfo()
{
}

bool OsmAnd::ObfInfo::containsDataFor(const AreaI& bbox31, const ZoomLevel minZoomLevel, const ZoomLevel maxZoomLevel) const
{
    //TODO: This needs to be implemented to allow skipping OBF files that do not contain any valuable data for given bbox
    return true;
    //for (const auto& mapSection : constOf(mapSections))
    //{
    //    for (const auto& level : constOf(mapSection->levels))
    //    {
    //        bool accept = false;

    //        // Check by zoom
    //        accept = accept || (minZoomLevel <= level->maxZoom && level->minZoom <= maxZoomLevel);

    //        // Check by area
    //        accept = accept || level->area31.contains(bbox31);
    //        accept = accept || level->area31.intersects(bbox31);
    //        accept = accept || bbox31.contains(level->area31);

    //        if (accept)
    //            return true;
    //    }
    //}

    //for (const auto& routingSection : constOf(routingSections))
    //{
    //    for (const auto& level : constOf(routingSection->levels))
    //    {
    //        bool accept = false;

    //        // Check by zoom
    //        accept = accept || (minZoomLevel <= level->maxZoom && level->minZoom <= maxZoomLevel);

    //        // Check by area
    //        accept = accept || level->area31.contains(bbox31);
    //        accept = accept || level->area31.intersects(bbox31);
    //        accept = accept || bbox31.contains(level->area31);

    //        if (accept)
    //            return true;
    //    }
    //}

    //for (const auto& poiSection : constOf(poiSections))
    //{
    //    bool accept = false;

    //    // Check by area
    //    accept = accept || poiSection->area31.contains(bbox31);
    //    accept = accept || poiSection->area31.intersects(bbox31);
    //    accept = accept || bbox31.contains(poiSection->area31);

    //    if (accept)
    //        return true;
    //}

    //return false;
}
