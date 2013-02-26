package net.osmand.plus;


import java.util.ArrayList;
import java.util.List;

import net.osmand.Location;
import net.osmand.data.LatLon;
import net.osmand.plus.routing.RoutingHelper;

public class TargetPointsHelper {

	private List<LatLon> intermediatePoints = new ArrayList<LatLon>(); 
	private List<String> intermediatePointNames = new ArrayList<String>();
	private LatLon pointToNavigate = null;
	private OsmandSettings settings;
	private RoutingHelper routingHelper;
	
	public TargetPointsHelper(OsmandSettings settings, RoutingHelper routingHelper){
		this.settings = settings;
		this.routingHelper = routingHelper;
		readFromSettings(settings);
	}

	private void readFromSettings(OsmandSettings settings) {
		pointToNavigate = settings.getPointToNavigate();
		intermediatePoints.clear();
		intermediatePointNames.clear();
		intermediatePoints.addAll(settings.getIntermediatePoints());
		intermediatePointNames.addAll(settings.getIntermediatePointDescriptions(intermediatePoints.size()));
	}
	
	public LatLon getPointToNavigate() {
		return pointToNavigate;
	}
	
	public String getPointNavigateDescription(){
		return settings.getPointNavigateDescription();
	}
	
	public List<String> getIntermediatePointNames() {
		return intermediatePointNames;
	}
	
	public List<LatLon> getIntermediatePoints() {
		return intermediatePoints;
	}
	
	public List<LatLon> getIntermediatePointsWithTarget() {
		List<LatLon> res = new ArrayList<LatLon>();
		res.addAll(intermediatePoints);
		if(pointToNavigate != null) {
			res.add(pointToNavigate);
		}
		return res;
	}
	
	public List<String> getIntermediatePointNamesWithTarget() {
		List<String> res = new ArrayList<String>();
		res.addAll(intermediatePointNames);
		if(pointToNavigate != null) {
			res.add(getPointNavigateDescription());
		}
		return res;
	}

	public LatLon getFirstIntermediatePoint(){
		if(intermediatePoints.size() > 0) {
			return intermediatePoints.get(0);
		}
		return null;
	}
	
	/**
	 * Clear the local and persistent waypoints list and destination.
	 */
	public void removeAllWayPoints(MapScreen map, boolean updateRoute){
		settings.clearPointToNavigate();
		pointToNavigate = null;		
		settings.clearIntermediatePoints();
		settings.clearPointToNavigate(); 
		intermediatePoints.clear();
		intermediatePointNames.clear();	
		updateRouteAndReferesh(map, updateRoute);
	}

	/**
	 * Move an intermediate waypoint to the destination.
	 */
	public void makeWayPointDestination(MapScreen map, boolean updateRoute, int index){
		pointToNavigate = intermediatePoints.remove(index);
		settings.setPointToNavigate(pointToNavigate.getLatitude(), pointToNavigate.getLongitude(), 
				intermediatePointNames.remove(index));		
		settings.deleteIntermediatePoint(index);
		updateRouteAndReferesh(map, updateRoute);
	}

	public void removeWayPoint(MapScreen map, boolean updateRoute, int index){
		if(index < 0){
			settings.clearPointToNavigate();
			pointToNavigate = null;
			int sz = intermediatePoints.size();
			if(sz > 0) {
				settings.deleteIntermediatePoint(sz- 1);
				pointToNavigate = intermediatePoints.remove(sz - 1);
				settings.setPointToNavigate(pointToNavigate.getLatitude(), pointToNavigate.getLongitude(), 
						intermediatePointNames.remove(sz - 1));
			}
		} else {
			settings.deleteIntermediatePoint(index);
			intermediatePoints.remove(index);
			intermediatePointNames.remove(index);	
		}
		updateRouteAndReferesh(map, updateRoute);
	}

	private void updateRouteAndReferesh(MapScreen map, boolean updateRoute) {
		if(updateRoute && ( routingHelper.isRouteBeingCalculated() || routingHelper.isRouteCalculated() ||
				routingHelper.isFollowingMode())) {
			Location lastKnownLocation = map == null ? routingHelper.getLastProjection() :  map.getLastKnownLocation();
			routingHelper.setFinalAndCurrentLocation(settings.getPointToNavigate(),
					settings.getIntermediatePoints(), lastKnownLocation, routingHelper.getCurrentGPXRoute());
		}
		if(map != null) {
			map.refreshMap();
		}
	}
	
	
	public void clearPointToNavigate(MapScreen map, boolean updateRoute) {
		settings.clearPointToNavigate();
		settings.clearIntermediatePoints();
		readFromSettings(settings);
		updateRouteAndReferesh(map, updateRoute);
	}
	
	
	public void reorderAllTargetPoints(MapScreen map, List<LatLon> point, 
			List<String> names, boolean updateRoute){
		settings.clearPointToNavigate();
		if (point.size() > 0) {
			settings.saveIntermediatePoints(point.subList(0, point.size() - 1), names.subList(0, point.size() - 1));
			LatLon p = point.get(point.size() - 1);
			String nm = names.get(point.size() - 1);
			settings.setPointToNavigate(p.getLatitude(), p.getLongitude(), nm);
		} else {
			settings.clearIntermediatePoints();
		}
		readFromSettings(settings);
		updateRouteAndReferesh(map, updateRoute);
	}
	
	public void navigateToPoint(MapScreen map, LatLon point, boolean updateRoute, int intermediate){
		if(point != null){
			if(intermediate < 0) {
				settings.setPointToNavigate(point.getLatitude(), point.getLongitude(), null);
			} else {
				settings.insertIntermediatePoint(point.getLatitude(), point.getLongitude(), null, 
						intermediate, false);
			}
		} else {
			settings.clearPointToNavigate();
			settings.clearIntermediatePoints();
		}
		readFromSettings(settings);
		updateRouteAndReferesh(map, updateRoute);
		
	}
	
	public boolean checkPointToNavigate(ClientContext ctx ){
    	if(pointToNavigate == null){
    		ctx.showToastMessage(R.string.mark_final_location_first);
			return false;
		}
    	return true;
    }

	public void updatePointsFromSettings() {
		readFromSettings(settings);		
	}

	
}
