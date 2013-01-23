package net.osmand.router;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;

import net.osmand.LogUtil;
import net.osmand.NativeLibrary;
import net.osmand.binary.BinaryMapIndexReader;
import net.osmand.router.BinaryRoutePlanner.FinalRouteSegment;
import net.osmand.router.BinaryRoutePlanner.RouteSegment;
import net.osmand.router.RoutingConfiguration.Builder;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public class RouterTestsSuite {
	
	public static int MEMORY_TEST_LIMIT = 800;
	public static boolean TEST_WO_HEURISTIC = true; 
	public static boolean TEST_BOTH_DIRECTION = true;
	
	private static class Parameters {
		public File obfDir;
		public List<File> tests = new ArrayList<File>();
		public RoutingConfiguration.Builder configBuilder;
		
		public static Parameters init(String[] args) throws IOException, XmlPullParserException {
			Parameters p = new Parameters();
			String routingXmlFile = null;
			String obfDirectory = null;
			RouteResultPreparation.PRINT_TO_CONSOLE_ROUTE_INFORMATION_TO_TEST = false;
			for (String a : args) {
				if (a.startsWith("-routingXmlPath=")) {
					routingXmlFile = a.substring("-routingXmlPath=".length());
				} else if (a.startsWith("-verbose")) {
					RouteResultPreparation.PRINT_TO_CONSOLE_ROUTE_INFORMATION_TO_TEST = true;
				} else if (a.startsWith("-obfDir=")) {
					obfDirectory = a.substring("-obfDir=".length());
				} else if (a.startsWith("-testDir=")) {
					String testDirectory = a.substring("-testDir=".length());
					for(File f : new File(testDirectory).listFiles()) {
						if(f.getName().endsWith(".test.xml")){
							p.tests.add(f);
						}
					}
				} else if(!a.startsWith("-")){
					p.tests.add(new File(a));
				}
			}

			if (obfDirectory == null) {
				throw new IllegalStateException("Directory with obf files not specified");
			}
			p.obfDir = new File(obfDirectory);

			if (routingXmlFile == null || routingXmlFile.equals("routing.xml")) {
				p.configBuilder = RoutingConfiguration.getDefault();
			} else {
				p.configBuilder = RoutingConfiguration.parseFromInputStream(new FileInputStream(routingXmlFile));
			}

			return p;
		}
	}
	
	public static void main(String[] args) throws Exception {
		if(args == null || args.length == 0) {
			info();
			return;
		}
		long time = System.currentTimeMillis();
		Parameters params = Parameters.init(args);
		if(params.tests.isEmpty() || params.obfDir == null) {
			info();
			return;
		}
		List<File> files = new ArrayList<File>();
		
		
		for (File f : params.obfDir.listFiles()) {
			if (f.getName().endsWith(".obf")) {
				files.add(f);
			}
		}
		System.out.println("Obf directory : ");
		BinaryMapIndexReader[] rs = new BinaryMapIndexReader[files.size()];
		int it = 0;
		for (File f : files) {
			RandomAccessFile raf = new RandomAccessFile(f.getAbsolutePath(), "r"); //$NON-NLS-1$ //$NON-NLS-2$
			System.out.println(f.getName());
			rs[it++] = new BinaryMapIndexReader(raf);
		}
		
		boolean allSuccess = true;
		for(File f : params.tests) {
			System.out.println("Before test " + f.getAbsolutePath());
			System.out.flush();
			allSuccess &= test(null, new FileInputStream(f), rs, params.configBuilder);	
		}
		if (allSuccess) {
			System.out.println("All is successfull " + (System.currentTimeMillis() - time) + " ms");
		}

	}


	private static void info() {
		println("Run router tests is console utility to test route calculation for osmand.");
		println("\nUsage for run tests : runTestsSuite [-routingXmlPath=PATH] [-verbose]  [-obfDir=PATH] [-testDir=PATH] {individualTestPath}");
		return;
	}
	

	private static void println(String string) {
		System.out.println(string);
	}


	public static boolean test(NativeLibrary lib, InputStream resource, BinaryMapIndexReader[] rs, RoutingConfiguration.Builder config) throws Exception {
		XmlPullParser parser = LogUtil.newXMLPullParser();
		parser.setInput(resource, "UTF-8");
		int tok;
		while ((tok = parser.next()) != XmlPullParser.END_DOCUMENT) {
			if (tok == XmlPullParser.START_TAG) {
				String name = parser.getName();
				if(name.equals("test")){
					testRoute(parser, config, lib, rs);
				}
			}
		}

		return true;
	}
	
	private static float parseFloat(XmlPullParser parser, String attr) {
		String v = parser.getAttributeValue("", attr);
		if(v == null || v.length() == 0){
			return 0;
		}
		return Float.parseFloat(v);
	}
	private static boolean isInOrLess(float expected, float value, float percent){
		if(equalPercent(expected, value, percent)){
			return true;
		}
		if(value < expected) {
			System.err.println("Test could be adjusted value "  + value + " is much less then expected " + expected);
			return true;
		}
		return false;
	}
	
	private static boolean equalPercent(float expected, float value, float percent){
		if(Math.abs(value/expected - 1) < percent / 100){
			return true;
		}
		return false;
	}

	private static void testRoute(XmlPullParser parser, Builder config, NativeLibrary lib, BinaryMapIndexReader[] rs) throws IOException, InterruptedException {
		String vehicle = parser.getAttributeValue("", "vehicle");
		int loadedTiles = (int) parseFloat(parser, "loadedTiles");
		int visitedSegments = (int) parseFloat(parser, "visitedSegments");
		int complete_time = (int) parseFloat(parser, "complete_time");
		int routing_time = (int) parseFloat(parser, "routing_time");
		int complete_distance = (int) parseFloat(parser, "complete_distance");
		float percent = parseFloat(parser, "best_percent");
		String testDescription = parser.getAttributeValue("", "description");
		if(percent == 0){
			System.err.println("\n\n!! Skipped test case '" + testDescription + "' because 'best_percent' attribute is not specified \n\n" );
			return;
		}
		RoutingConfiguration rconfig = config.build(vehicle, MEMORY_TEST_LIMIT);
		RoutePlannerFrontEnd router = new RoutePlannerFrontEnd(false);
		RoutingContext ctx = new RoutingContext(rconfig, 
				lib, rs);
		String skip = parser.getAttributeValue("", "skip_comment");
		if (skip != null && skip.length() > 0) {
			System.err.println("\n\n!! Skipped test case '" + testDescription + "' because '" + skip + "'\n\n" );
			return;
		}
		System.out.println("Run test " + testDescription);
		
		double startLat = Double.parseDouble(parser.getAttributeValue("", "start_lat"));
		double startLon = Double.parseDouble(parser.getAttributeValue("", "start_lon"));
		RouteSegment startSegment = router.findRouteSegment(startLat, startLon, ctx);
		double endLat = Double.parseDouble(parser.getAttributeValue("", "target_lat"));
		double endLon = Double.parseDouble(parser.getAttributeValue("", "target_lon"));
		RouteSegment endSegment = router.findRouteSegment(endLat, endLon, ctx);
		if(startSegment == null){
			throw new IllegalArgumentException("Start segment is not found for test : " + testDescription);
		}
		if(endSegment == null){
			throw new IllegalArgumentException("End segment is not found for test : " + testDescription);
		}
		List<RouteSegmentResult> route = router.searchRoute(ctx, startSegment, endSegment, false);
		final float calcRoutingTime = ctx.routingTime;
		float completeTime = 0;
		float completeDistance = 0;
		for (int i = 0; i < route.size(); i++) {
			completeTime += route.get(i).getSegmentTime();
			completeDistance += route.get(i).getDistance();
		}
		if(complete_time > 0 && !isInOrLess(complete_time, completeTime, percent)) {
			throw new IllegalArgumentException(MessageFormat.format("Complete time (expected) {0} != {1} (original) : {2}", complete_time, completeTime, testDescription));
		}
		if(complete_distance > 0 && !isInOrLess(complete_distance, completeDistance, percent)) {
			throw new IllegalArgumentException(MessageFormat.format("Complete distance (expected) {0} != {1} (original) : {2}", complete_distance, completeDistance, testDescription));
		}
		if(routing_time > 0 && !isInOrLess(routing_time, calcRoutingTime, percent)) {
			throw new IllegalArgumentException(MessageFormat.format("Complete routing time (expected) {0} != {1} (original) : {2}", routing_time, calcRoutingTime, testDescription));
		}

		if (visitedSegments > 0 && !isInOrLess(visitedSegments, ctx.visitedSegments, percent)) {
			throw new IllegalArgumentException(MessageFormat.format("Visited segments (expected) {0} != {1} (original) : {2}", visitedSegments,
					ctx.visitedSegments, testDescription));
		}
		if (loadedTiles > 0 && !isInOrLess(loadedTiles, ctx.loadedTiles, percent)) {
			throw new IllegalArgumentException(MessageFormat.format("Loaded tiles (expected) {0} != {1} (original) : {2}", loadedTiles,
					ctx.loadedTiles, testDescription));
		}
		
		if(TEST_BOTH_DIRECTION){
			rconfig.planRoadDirection = -1;
			runTestSpecialTest(lib, rs, rconfig, router, startSegment, endSegment, calcRoutingTime, "Calculated routing time in both direction {0} != {1} time in -1 direction");
			
			rconfig.planRoadDirection = 1;
			runTestSpecialTest(lib, rs, rconfig, router, startSegment, endSegment, calcRoutingTime, "Calculated routing time in both direction {0} != {1} time in 1 direction");
			
		}
		
		if(TEST_WO_HEURISTIC) {
			rconfig.planRoadDirection = 0;
			rconfig.heuristicCoefficient = 0.5f;
			runTestSpecialTest(lib, rs, rconfig, router, startSegment, endSegment, calcRoutingTime, 
				"Calculated routing time with heuristic 1 {0} != {1} with heuristic 0.5");
		}
		
	}


	private static void runTestSpecialTest(NativeLibrary lib, BinaryMapIndexReader[] rs, RoutingConfiguration rconfig, RoutePlannerFrontEnd router,
			RouteSegment startSegment, RouteSegment endSegment, final float calcRoutingTime, String msg) throws IOException, InterruptedException {
		RoutingContext ctx;
		ctx = new RoutingContext(rconfig, lib, rs);
		router.searchRoute(ctx, startSegment, endSegment, false);
		FinalRouteSegment frs = ctx.finalRouteSegment;
		if(frs == null || !equalPercent(calcRoutingTime, frs.distanceFromStart, 0.5f)){
			throw new IllegalArgumentException(MessageFormat.format(msg, calcRoutingTime+"",frs == null?"0":frs.distanceFromStart+""));
		}
	}



}
