package net.osmand.core.samples.android.sample1;

import android.annotation.SuppressLint;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.PointF;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.ActionBarActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;

import net.osmand.core.android.AtlasMapRendererView;
import net.osmand.core.android.CoreResourcesFromAndroidAssets;
import net.osmand.core.android.NativeCore;
import net.osmand.core.jni.AmenitiesByNameSearch;
import net.osmand.core.jni.Amenity;
import net.osmand.core.jni.AreaI;
import net.osmand.core.jni.IMapLayerProvider;
import net.osmand.core.jni.IMapStylesCollection;
import net.osmand.core.jni.IObfsCollection;
import net.osmand.core.jni.IQueryController;
import net.osmand.core.jni.ISearch;
import net.osmand.core.jni.LatLon;
import net.osmand.core.jni.LogSeverityLevel;
import net.osmand.core.jni.Logger;
import net.osmand.core.jni.MapObjectsSymbolsProvider;
import net.osmand.core.jni.MapPresentationEnvironment;
import net.osmand.core.jni.MapPrimitivesProvider;
import net.osmand.core.jni.MapPrimitiviser;
import net.osmand.core.jni.MapRasterLayerProvider_Software;
import net.osmand.core.jni.MapStylesCollection;
import net.osmand.core.jni.ObfInfo;
import net.osmand.core.jni.ObfMapObjectsProvider;
import net.osmand.core.jni.ObfsCollection;
import net.osmand.core.jni.PointI;
import net.osmand.core.jni.QIODeviceLogSink;
import net.osmand.core.jni.QStringList;
import net.osmand.core.jni.QStringStringHash;
import net.osmand.core.jni.ResolvedMapStyle;
import net.osmand.core.jni.Utilities;
import net.osmand.core.samples.android.sample1.MultiTouchSupport.MultiTouchZoomListener;

public class MainActivity extends ActionBarActivity {
    private static final String TAG = "OsmAndCoreSample";

    private float _displayDensityFactor;
    private int _referenceTileSize;
    private int _rasterTileSize;
    private IMapStylesCollection _mapStylesCollection;
    private ResolvedMapStyle _mapStyle;
    private ObfsCollection _obfsCollection;
    private MapPresentationEnvironment _mapPresentationEnvironment;
    private MapPrimitiviser _mapPrimitiviser;
    private ObfMapObjectsProvider _obfMapObjectsProvider;
    private MapPrimitivesProvider _mapPrimitivesProvider;
    private MapObjectsSymbolsProvider _mapObjectsSymbolsProvider;
    private IMapLayerProvider _mapLayerProvider0;
    private IMapLayerProvider _mapLayerProvider1;
    private QIODeviceLogSink _fileLogSink;

	private AtlasMapRendererView _mapView;
	private TextView textZoom;
	private ImageButton azimuthNorthButton;

	private GestureDetector gestureDetector;
	private PointI target31;
	private float zoom;
	private float azimuth;
	private float elevationAngle;
	private MultiTouchSupport multiTouchSupport;

	private final static int MAX_RESULTS = 10;

	// Germany
	private final static float INIT_LAT = 49.353953f;
	private final static float INIT_LON = 11.214384f;
	// Kyiv
	//private final static float INIT_LAT = 50.450117f;
	//private final static float INIT_LON = 30.524142f;
	private final static float INIT_ZOOM = 6.0f;
	private final static float INIT_AZIMUTH = 0.0f;
	private final static float INIT_ELEVATION_ANGLE = 90.0f;
	private final static int MIN_ZOOM_LEVEL = 2;
	private final static int MAX_ZOOM_LEVEL = 22;
	private int resCount = 0;

	private static final String PREF_MAP_CENTER_LAT = "MAP_CENTER_LAT";
	private static final String PREF_MAP_CENTER_LON = "MAP_CENTER_LON";
	private static final String PREF_MAP_AZIMUTH = "MAP_AZIMUTH";
	private static final String PREF_MAP_ZOOM = "MAP_ZOOM";
	private static final String PREF_MAP_ELEVATION_ANGLE = "MAP_ELEVATION_ANGLE";

	@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		gestureDetector = new GestureDetector(this, new MapViewOnGestureListener());
		multiTouchSupport = new MultiTouchSupport(this, new MapViewMultiTouchZoomListener());

        long startTime = System.currentTimeMillis();

        // Initialize native core prior (if needed)
        if (NativeCore.isAvailable() && !NativeCore.isLoaded())
            NativeCore.load(CoreResourcesFromAndroidAssets.loadFromCurrentApplication(this));

		Logger.get().setSeverityLevelThreshold(LogSeverityLevel.Debug);

        // Inflate views
        setContentView(R.layout.activity_main);

        // Get map view
        _mapView = (AtlasMapRendererView) findViewById(R.id.mapRendererView);

		textZoom = (TextView) findViewById(R.id.text_zoom);
		azimuthNorthButton = (ImageButton) findViewById(R.id.map_azimuth_north_button);

		azimuthNorthButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setAzimuth(0f);
			}
		});

		findViewById(R.id.map_zoom_in_button).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setZoom(zoom + 1f);
			}
		});

		findViewById(R.id.map_zoom_out_button).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setZoom(zoom - 1f);
			}
		});

        // Additional log sink
        _fileLogSink = QIODeviceLogSink.createFileLogSink(
                Environment.getExternalStorageDirectory() + "/osmand/osmandcore.log");
        Logger.get().addLogSink(_fileLogSink);

        // Get device display density factor
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        _displayDensityFactor = displayMetrics.densityDpi / 160.0f;
        _referenceTileSize = (int)(256 * _displayDensityFactor);
        _rasterTileSize = Integer.highestOneBit(_referenceTileSize - 1) * 2;
        Log.i(TAG, "displayDensityFactor = " + _displayDensityFactor);
        Log.i(TAG, "referenceTileSize = " + _referenceTileSize);
        Log.i(TAG, "rasterTileSize = " + _rasterTileSize);

        Log.i(TAG, "Going to resolve default embedded style...");
        _mapStylesCollection = new MapStylesCollection();
        _mapStyle = _mapStylesCollection.getResolvedStyleByName("default");
        if (_mapStyle == null)
        {
            Log.e(TAG, "Failed to resolve style 'default'");
            System.exit(0);
        }

        Log.i(TAG, "Going to prepare OBFs collection");
        _obfsCollection = new ObfsCollection();
        Log.i(TAG, "Will load OBFs from " + Environment.getExternalStorageDirectory() + "/osmand");
        _obfsCollection.addDirectory(Environment.getExternalStorageDirectory() + "/osmand", false);

        Log.i(TAG, "Going to prepare all resources for renderer");
        _mapPresentationEnvironment = new MapPresentationEnvironment(
                _mapStyle,
                _displayDensityFactor,
                1.0f,
                1.0f,
                "en"); //TODO: here should be current locale
        //mapPresentationEnvironment->setSettings(configuration.styleSettings);
        _mapPrimitiviser = new MapPrimitiviser(
                _mapPresentationEnvironment);
        _obfMapObjectsProvider = new ObfMapObjectsProvider(
                _obfsCollection);
        _mapPrimitivesProvider = new MapPrimitivesProvider(
                _obfMapObjectsProvider,
                _mapPrimitiviser,
                _rasterTileSize);
        _mapObjectsSymbolsProvider = new MapObjectsSymbolsProvider(
                _mapPrimitivesProvider,
                _rasterTileSize);

        _mapView.setReferenceTileSizeOnScreenInPixels(_referenceTileSize);
        _mapView.addSymbolsProvider(_mapObjectsSymbolsProvider);

		restoreMapState();

        _mapLayerProvider0 = new MapRasterLayerProvider_Software(_mapPrimitivesProvider);
        _mapView.setMapLayerProvider(0, _mapLayerProvider0);

		System.out.println("NATIVE_INITIALIZED = " + (System.currentTimeMillis() - startTime) / 1000f);

		//runSearch();
	}

    @Override
    protected void onResume() {
        super.onResume();

        _mapView.handleOnResume();
    }

    @Override
    protected void onPause() {
		saveMapState();
        _mapView.handleOnPause();

        super.onPause();
    }

    @Override
    protected void onDestroy() {
        _mapView.handleOnDestroy();

        super.onDestroy();
    }

	public void saveMapState() {
		SharedPreferences prefs = getPreferences(MODE_PRIVATE);
		Editor edit = prefs.edit();
		LatLon latLon = Utilities.convert31ToLatLon(target31);
		edit.putFloat(PREF_MAP_CENTER_LAT, (float)latLon.getLatitude());
		edit.putFloat(PREF_MAP_CENTER_LON, (float)latLon.getLongitude());
		edit.putFloat(PREF_MAP_AZIMUTH, azimuth);
		edit.putFloat(PREF_MAP_ZOOM, zoom);
		edit.putFloat(PREF_MAP_ELEVATION_ANGLE, elevationAngle);
		edit.commit();
	}

	public void restoreMapState() {
		SharedPreferences prefs = getPreferences(MODE_PRIVATE);
		float prefLat = prefs.getFloat(PREF_MAP_CENTER_LAT, INIT_LAT);
		float prefLon = prefs.getFloat(PREF_MAP_CENTER_LON, INIT_LON);
		float prefAzimuth = prefs.getFloat(PREF_MAP_AZIMUTH, INIT_AZIMUTH);
		float prefZoom = prefs.getFloat(PREF_MAP_ZOOM, INIT_ZOOM);
		float prefElevationAngle = prefs.getFloat(PREF_MAP_ELEVATION_ANGLE, INIT_ELEVATION_ANGLE);

		setAzimuth(prefAzimuth);
		setElevationAngle(prefElevationAngle);
		setTarget(Utilities.convertLatLonTo31(new LatLon(prefLat, prefLon)));
		setZoom(prefZoom);
	}

	public boolean setTarget(PointI pointI) {
		target31 = pointI;
		return _mapView.setTarget(pointI);
	}

	@SuppressLint("DefaultLocale")
	public boolean setZoom(float zoom) {
		if (zoom < MIN_ZOOM_LEVEL) {
			zoom = MIN_ZOOM_LEVEL;
		} else if (zoom > MAX_ZOOM_LEVEL) {
			zoom = MAX_ZOOM_LEVEL;
		}
		this.zoom = zoom;
		textZoom.setText(String.format("%.0f", zoom));
		return _mapView.setZoom(zoom);
	}

	public void setAzimuth(float angle) {
		angle = MapUtils.unifyRotationTo360(angle);
		this.azimuth = angle;
		_mapView.setAzimuth(angle);

		if (angle == 0f && azimuthNorthButton.getVisibility() == View.VISIBLE) {
			azimuthNorthButton.setVisibility(View.INVISIBLE);
		} else if (angle != 0f && azimuthNorthButton.getVisibility() == View.INVISIBLE) {
			azimuthNorthButton.setVisibility(View.VISIBLE);
		}
	}

	public void setElevationAngle(float angle) {
		if (angle < 35f) {
			angle = 35f;
		} else if (angle > 90f) {
			angle = 90f;
		}
		this.elevationAngle = angle;
		_mapView.setElevationAngle(angle);
	}

	public boolean onTouchEvent(MotionEvent event) {
		return multiTouchSupport.onTouchEvent(event)
				|| gestureDetector.onTouchEvent(event);
	}

	private void runSearch() {
		System.out.println("=== Start search");
		resCount = 0;

		AmenitiesByNameSearch byNameSearch = new AmenitiesByNameSearch(_obfsCollection);
		AmenitiesByNameSearch.Criteria criteria = new AmenitiesByNameSearch.Criteria();
		criteria.setName("bank");

		/*
		PointI topLeftPoint = new PointI();
		PointI bottomRightPoint = new PointI();
		_mapView.getLocationFromScreenPoint(new PointI(0, 0), topLeftPoint);
		_mapView.getLocationFromScreenPoint(new PointI(_mapView.getWidth(), _mapView.getHeight()), bottomRightPoint);
		*/

		// Kyiv
		PointI topLeftPoint = Utilities.convertLatLonTo31(new LatLon(50.498509, 30.445950));
		PointI bottomRightPoint = Utilities.convertLatLonTo31(new LatLon(50.400741, 30.577443));

		final AreaI visibleArea = new AreaI(topLeftPoint, bottomRightPoint);

		IObfsCollection.IAcceptorFunction acceptorFunction = new IObfsCollection.IAcceptorFunction() {
			@Override
			public boolean method(ObfInfo obfInfo) {

				boolean res = obfInfo.containsPOIFor(visibleArea);
				return res && resCount < MAX_RESULTS;
			}
		};

		criteria.setSourceFilter(acceptorFunction.getBinding());

		ISearch.INewResultEntryCallback newResultEntryCallback = new ISearch.INewResultEntryCallback() {
			@Override
			public void method(ISearch.Criteria criteria, ISearch.IResultEntry resultEntry) {
				Amenity amenity = new ResultEntry(resultEntry).getAmenity();
				System.out.println("Poi found === " + amenity.getNativeName());
				QStringStringHash locNames = amenity.getLocalizedNames();
				if (locNames.size() > 0) {
					QStringList keys = locNames.keys();
					StringBuilder sb = new StringBuilder("=== Localized names: ");
					for (int i = 0; i < keys.size(); i++) {
						String key = keys.get(i);
						sb.append(key).append("=").append(locNames.get(key)).append(" | ");
					}
					System.out.println(sb.toString());
				}
				resCount++;
			}
		};

		byNameSearch.performSearch(criteria, newResultEntryCallback.getBinding(), new IQueryController() {
			@Override
			public boolean isAborted() {
				return resCount >= MAX_RESULTS;
			}
		});

		System.out.println("=== Finish search");
	}

	private static class ResultEntry extends AmenitiesByNameSearch.ResultEntry {
		protected ResultEntry(ISearch.IResultEntry resultEntry) {
			super(ISearch.IResultEntry.getCPtr(resultEntry), false);
		}
	}

	private class MapViewOnGestureListener extends SimpleOnGestureListener {

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
			float fromX = e2.getX() + distanceX;
			float fromY = e2.getY() + distanceY;
			float toX = e2.getX();
			float toY = e2.getY();

			float dx = (fromX - toX);
			float dy = (fromY - toY);

			PointI newTarget = new PointI();
			_mapView.getLocationFromScreenPoint(new PointI(_mapView.getWidth() / 2 + (int)dx, _mapView.getHeight() / 2 + (int)dy), newTarget);

			setTarget(newTarget);

			return true;
		}
	}

	private class MapViewMultiTouchZoomListener implements MultiTouchZoomListener {

		private float initialZoom;
		private float initialAzimuth;
		private float initialElevation;
		private PointF centerPoint;

		@Override
		public void onGestureFinished(float scale, float rotation) {
		}

		@Override
		public void onGestureInit(float x1, float y1, float x2, float y2) {
		}

		@Override
		public void onZoomStarted(PointF centerPoint) {
			initialZoom = zoom;
			initialAzimuth = azimuth;
			this.centerPoint = centerPoint;
		}

		@Override
		public void onZoomingOrRotating(float scale, float rotation) {

			PointI centerLocationBefore = new PointI();
			_mapView.getLocationFromScreenPoint(
					new PointI((int)centerPoint.x, (int)centerPoint.y), centerLocationBefore);

			// Change zoom
			setZoom(initialZoom + (float)(Math.log(scale) / Math.log(2)));

			// Adjust current target position to keep touch center the same
			PointI centerLocationAfter = new PointI();
			_mapView.getLocationFromScreenPoint(
					new PointI((int)centerPoint.x, (int)centerPoint.y), centerLocationAfter);
			PointI centerLocationDelta = new PointI(
					centerLocationAfter.getX() - centerLocationBefore.getX(),
					centerLocationAfter.getY() - centerLocationBefore.getY());

			setTarget(new PointI(target31.getX() - centerLocationDelta.getX(), target31.getY() - centerLocationDelta.getY()));

			/*
			// Convert point from screen to location
			PointI centerLocation = new PointI();
			_mapView.getLocationFromScreenPoint(
					new PointI((int)centerPoint.x, (int)centerPoint.y), centerLocation);

			// Rotate current target around center location
			PointI target = new PointI(xI - centerLocation.getX(), yI - centerLocation.getY());
			double cosAngle = Math.cos(-Math.toRadians(rotation));
			double sinAngle = Math.sin(-Math.toRadians(rotation));

			PointI newTarget = new PointI(
					(int)(target.getX() * cosAngle - target.getY() * sinAngle + centerLocation.getX()),
					(int)(target.getX() * sinAngle + target.getY() * cosAngle + centerLocation.getY()));

			setTarget(newTarget);
			*/

			// Set rotation
			setAzimuth(initialAzimuth - rotation);
		}

		@Override
		public void onChangeViewAngleStarted() {
			initialElevation = elevationAngle;
		}

		@Override
		public void onChangingViewAngle(float angle) {
			setElevationAngle(initialElevation - angle);
		}
	}
}
