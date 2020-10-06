//
//  routeDataResources.h
//  OsmAndCore_static_standalone
//
//  Created by nnngrach on 03.10.2020.
//

#ifndef _OSMAND_ROUTE_DATA_RESOURCES_H
#define _OSMAND_ROUTE_DATA_RESOURCES_H

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include "binaryRead.h"
#include <routeTypeRule.h>
#include "Location.h"
#include <QMap>

struct RouteDataResources {
private:
    QMap<std::shared_ptr<RouteTypeRule>, int> rules;
    std::vector<std::shared_ptr<OsmAnd::Location>> locations;
    int currentLocation;
    QMap<std::shared_ptr<RouteDataObject>, std::vector<std::vector<uint32_t>>> pointNamesMap;
   
public:
    RouteDataResources()
    {
        std::vector<std::shared_ptr<OsmAnd::LatLon>> location;
    }
    
    RouteDataResources(std::vector<std::shared_ptr<OsmAnd::Location>> locations)
    {
        this->locations = locations;
    }
    
    inline QMap<std::shared_ptr<RouteTypeRule>, int> getRules() const {
        return rules;
    }
    
    inline std::vector<std::shared_ptr<OsmAnd::Location>> getLocations() const {
        return locations;
    }
    
    inline bool hasLocations() const {
        return locations.size() > 0;
    }
    
    inline std::shared_ptr<OsmAnd::Location> getLocation(int index) const {
        index += currentLocation;
        return index < locations.size() ? locations[index] : nullptr;
    }
    
    inline void incrementCurrentLocation(int index) {
        currentLocation += index;
    }
    
    inline QMap<std::shared_ptr<RouteDataObject>, std::vector<std::vector<uint32_t>>> getPointNamesMap() const {
        return pointNamesMap;
    }
    
};

#endif /*_OSMAND_ROUTE_DATA_RESOURCES_H*/
